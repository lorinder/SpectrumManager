#!/usr/bin/env python3

import json
import sys
import subprocess
import time

import mysql.connector as db

def _insert_obj_into_db(cursor, table, known_cols, wrinfo_id, D):
    kv_pairs = []
    if wrinfo_id is not None:
        kv_pairs.append( ('wrinfo_id', wrinfo_id) )
    for k, v in D.items():
        if k in known_cols:
            kv_pairs.append( (k, v) )
    cmd = "INSERT INTO %s (%s) VALUES (%s)" % \
            (table,
             ", ".join(x for x, y in kv_pairs),
             ", ".join(len(kv_pairs) * ["%s"]))
    attr_values = tuple([y for x, y in kv_pairs])
    cursor.execute(cmd, attr_values)

def _insert_objarr_into_db(cursor, table, known_cols, wrinfo_id, A):
    for D in A:
        _insert_obj_into_db(cursor, table, known_cols, wrinfo_id, D)

def _insert_ifinfo(cursor, wrinfo_id, D):
    known_cols = {  'ifindex',
                    'mac',
                    'ssid',
                    'frequency', 
                    'channel_width_enum',
                    'center_freq1',
                    'center_freq2',
                    'channel_type_enum' }
    _insert_obj_into_db(cursor,
                    "wrinfo_interface",
                    known_cols,
                    wrinfo_id,
                    D)

def _insert_metainfo(cursor, wrinfo_id, D):
    _insert_obj_into_db(cursor,
                    "wrinfo_meta",
                    { "cmd", "time_human", "time_unix" },
                    wrinfo_id,
                    D)

def _insert_scan(cursor, wrinfo_id, A):
    _insert_objarr_into_db(cursor,
                    "wrinfo_scan",
                    { "bssid", "frequency", "ssid" },
                    wrinfo_id, A)

def _insert_survey(cursor, wrinfo_id, A):
    _insert_objarr_into_db(cursor,
                    "wrinfo_survey",
                    { "frequency", "in_use", "noise", "time", "busy",
                      "rx", "tx" },
                    wrinfo_id, A)

def _get_wrinfo_cmd(conn, cursor, if_id, do_scan):
    # Find the associated ap_id and ifname
    cursor.execute("SELECT ap_id, ifname, measuring, wrinfo_cmd " +
                "FROM radio_if WHERE id=%s", (if_id,))
    result = list(cursor) #[row for row in cursor]
    if len(result) == 0:
        sys.stderr.write("Error:  Radio Interface %d does not exist.\n"
          % (if_id,))
        return (False, None)
    assert len(result) == 1
    if not result[0][2]:
        sys.stderr.write(
            "Info:  Interface is not being measured, skipping.\n")
        return (True, None)
    ap_id = result[0][0] 
    ifname = result[0][1]
    cmd = result[0][3]

    # Check if we can use a premade command
    if cmd is not None:
        return (True, cmd.split())

    # Find the IP address
    cursor.execute("SELECT ip_addr, in_use FROM ap WHERE id=%s", (ap_id,))
    result = [row for row in cursor]
    assert len(result) == 1
    ip_addr = result[0][0]
    if not result[0][1]:
        sys.stderr.write(
            "Info:  AP holding interface %d is not in use, skipping.\n"
            % (if_id,))
        return (True, None)

    # Construct ssh command
    cmd = [ "ssh", "-n", "-T", "root@%s" % (ip_addr,),
            "./wrinfo", "-i", ifname]
    if do_scan:
        cmd.append("-s")
    return (True, cmd)

def run_wrinfo(dbname, if_id, timeout, cmdline, do_scan, tag):
    # Open DB
    conn = db.connect(user='wifispecman',
                      password='password',
                      database=dbname)
    cursor = conn.cursor()

    # Create rungroup
    cursor.execute(
      "INSERT INTO rungroup (cmd, servertime, tag) "
      + "VALUES (%s, %s, %s)", (cmdline, int(time.time()), tag))
    rungroup_id = cursor.getlastrowid()

    # Run wrinfo
    succ, cmd = _get_wrinfo_cmd(conn, cursor, if_id, do_scan)
    if succ == False or cmd is None:
        return succ
    status = 0
    try:
        result = subprocess.run(cmd,
                    stdin=subprocess.DEVNULL,
                    capture_output=True,
                    text=True,
                    timeout=timeout)
    except subprocess.TimeoutExpired:
        status = 2

    exit_code = result.returncode
    if status == 0 and exit_code != 0:
        status = 1

    # Create the wrinfo entry
    cursor.execute(
      ("INSERT INTO wrinfo"
      + " (radio_if_id, status, exit_code, servertime, rungroup_id)"
      + " VALUES (%s, %s, %s, %s, %s)"),
      (if_id, status, exit_code, int(time.time()), rungroup_id)
    )
    wrinfo_id = cursor.getlastrowid()

    # Insert stderr
    for l in result.stderr.split('\n'):
        if l == "":
            continue
        cursor.execute(("INSERT INTO wrinfo_errors (wrinfo_id, msg)"
          + " VALUES (%s, %s)"), (wrinfo_id, l))

    # Parse JSON output and insert
    output_ok = True
    try:
        D = json.loads(result.stdout)
    except:
        output_ok = False
    else: 
        if type(D) == dict:
            if "interface" in D and type(D["interface"]) == dict:
                _insert_ifinfo(cursor, wrinfo_id, D["interface"])
            else:
                print("Error reading the interface section.")
                output_ok = False
            if "meta" in D and type(D["meta"]) == dict:
                _insert_metainfo(cursor, wrinfo_id, D["meta"])
            else:
                print("Error reading the meta section.")
                output_ok = False
            if "scan" in D and type(D["scan"]) == list:
                _insert_scan(cursor, wrinfo_id, D["scan"])
            else:
                print("Error reading the scan section.")
                output_ok = False
            if "survey" in D and type(D["survey"]) == list:
                _insert_survey(cursor, wrinfo_id, D["survey"])
            else:
                print("Error reading the survey section.")
                output_ok = False
        else:
            output_ok = False

    # Update wrinfo with successful
    if output_ok:
        print("--> all good.")
    else:
        if status == 0:
            # update in error case.
            cursor.execute(("UPDATE wrinfo SET status = %s WHERE " +
                            "wrinfo_id = %d" % (wrinfo_id,)), (3,))

    # Commit updates
    conn.commit()

def usage():
    print(  "run_wrinfo\n"
            "\n"
            "utility to run wrinfo on a remote host and update the\n"
            "database with resulting output\n"
            "\n"
            "   -h         display help and exit\n"
            "   -d <name>  connect to database with given name [wifispecman]\n"
            "   -i #       interface ID (radio_if.id value from DB)\n"
            "   -s         run channel scan as well (pass -s to wrinfo)\n"
            "   -t #       timeout in seconds\n"
            "   -T <name>  tag name to associate with run\n"
    )

if __name__ == "__main__":
    import getopt

    # Default settings
    if_id = None
    timeout = 60.0
    dbname = "wifispecman"
    do_scan = False
    tag = None

    # Parse cmdline args
    opts, args = getopt.getopt(sys.argv[1:], "hd:i:st:T:")
    for k, v in opts:
        if k == '-h':
            usage()
        elif k == '-d':
            dbname = v
        elif k == '-i':
            if_id = int(v)
        elif k == '-s':
            do_scan = True
        elif k == '-t':
            timeout = float(v)
        elif k == '-T':
            tag = v
        else:
            sys.exit(1)

    if if_id is None:
        sys.stderr.write("Error:  Missing interface ID (specify it with -i)\n")
        sys.exit(1)

    run_wrinfo(dbname, if_id, timeout, ' '.join(sys.argv), do_scan, tag)
