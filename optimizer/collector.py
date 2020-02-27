#!/usr/bin/env python3

"""
Data collection utility for the optimization algorithm.

This module (both a library and standalone utility, for testing.)
extracts and preprocesses data from the database needed by the
optimisation algorithm.
"""

import bisect
import sys
import time

import mysql.connector as db

# Size of the history in days
hist_size = 1

def open_db(dbname = None):
    if dbname is None:
        dbname = 'wifispecman'
    conn = db.connect(user='wifispecman',
                    password='password',
                    database=dbname)
    cursor = conn.cursor()
    return (conn, cursor)

def get_nodes(conn, cursor):
    """Compute the set of nodes to optimize.

    @return     List of tuples; each tuple corresponds to a
                node:  (radio_if_id, mac_addr)
    """

    sql = """SELECT radio_if.id, wrinfo_interface.mac
    FROM radio_if, wrinfo, wrinfo_interface
    WHERE wrinfo.radio_if_id = radio_if.id
    AND wrinfo.wrinfo_id = wrinfo_interface.wrinfo_id
    AND optimising = 1
    GROUP BY radio_if.id"""
    # XXX: Filter out deactivated APs in the above?
    cursor.execute(sql)
    return list(cursor)

def get_graph(conn, cursor, nodes):
    """Return the connectivity graph of the nodes.

    The graph is a dict u -> [ neighbors of u ], where all the node
    identifiers (u and the neighbors of u) are indexes into the nodes
    array.

    @param  nodes
            the nodes array as computed by get_nodes().
    """

    mac2index = dict([(mac, i) for (i, (ifid, mac)) in enumerate(nodes)])
    graph = dict()

    sql = """SELECT mac, bssid
    FROM wrinfo_interface, wrinfo_scan
    WHERE wrinfo_interface.wrinfo_id = wrinfo_scan.wrinfo_id
    GROUP BY mac, bssid"""
    cursor.execute(sql)

    for x, y in cursor:
        # Convert to radio_if number
        # skip if no such interface
        x = mac2index[x]
        if not y in mac2index:
            continue
        y = mac2index[y]

        # Add edges
        #
        # We add bidirectional edges since we assume that
        # interference is bidirectional.
        neigh = graph.get(x, [])
        if y not in neigh:
            neigh.append(y)
        graph[x] = neigh

        neigh = graph.get(y, [])
        if x not in neigh:
            neigh.append(x)
        graph[y] = neigh
    return graph

def get_ip_ifname_for_ifaces(conn, cursor, ifaces):
    """For interface IDs, produce IP address and interface name.

    @param  ifaces
            Sequence of interface IDs (keys to radio_id.id in DB)

    @return Generator producing tuples (ip_addr, ifname) in the same
            order as the input interface IDs.
    """
    sql = """SELECT radio_if.id, ip_addr, ifname
    FROM ap, radio_if
    WHERE ap.id = ap_id"""
    cursor.execute(sql)
    d = {}
    for (if_id, ip_addr, ifname) in list(cursor):
        yield (ip_addr, ifname)


class ChannelMetrics(object):
    """Stores computed channel metrics for a time slice.

    Channel Metrics are computed from channel statistics rows; the exact
    method of how they're computed depends on the survey type.

    ChannelMetrics are supposed to contain "cleaned" statistics, unlike the 
    ChannelStatsRow objects, which contain the raw database content.
    """
    def __init__(self, delta_wall, is_valid, delta_t, f_busy, f_rx, f_tx):
        self.delta_wall = delta_wall
        self.is_valid = is_valid
        self.delta_t = delta_t
        self.f_busy = f_busy
        self.f_rx = f_rx
        self.f_tx = f_tx

class ChannelStatsRow(object):
    """Stores one row of channel stats (walltime, time, busy, rx, tx).
    """

    def __init__(self, row):
        self.walltime, self.time, self.busy, self.rx, self.tx = row

    def __str__(self):
        s = "walltime=%d, time=%d, busy=%d, rx=%d, tx=%d" % \
            (self.walltime, self.time, self.busy, self.rx, self.tx)
        return s

    def __sub__(self, b):
        return ChannelStatsRow(
                self.walltime - b.walltime,
                self.time - b.time,
                self.busy - b.busy,
                self.rx - b.rx,
                self.tx - b.tx)

    def __lt__(self, b):
        return self.walltime < b.walltime

    def __le__(self, b):
        return self.walltime <= b.walltime

def get_survey_type(conn, cursor, radio_if_id):
    sql="SELECT survey_type FROM radio_if WHERE id = %d" \
        % (radio_if_id,)
    cursor.execute(sql)
    return list(cursor)[0][0]

class ChannelStatsRows(object):
    """An set of channel stats rows.

    Has two members:
        1) survey_type  the type of survey collected from that node, see DB
                        documentation for details.
        2) stats_rows   ordered list of ChannelStatsRows.
    """

    def __init__(self, conn, cursor, radio_if_id, freq, time_cutoff):

        # Get the stats rows themselves
        sql="""SELECT servertime, time, busy, rx, tx
        FROM wrinfo, wrinfo_survey
        WHERE wrinfo.wrinfo_id = wrinfo_survey.wrinfo_id
        AND frequency = %d
        AND radio_if_id = %d
        AND servertime >= %d
        ORDER BY wrinfo.wrinfo_id;""" \
          % (freq, radio_if_id, time_cutoff)
        cursor.execute(sql)
        self.stats_rows = [ ChannelStatsRow(row) for row in cursor ]

        # Get the survey type
        self.survey_type = get_survey_type(conn, cursor, radio_if_id)

    def find_index_for_time(self, time):
        """Find the index of a statistics row, given time."""
        R = self.stats_rows
        entry = ChannelStatsRow( (time, 0, 0, 0, 0) )
        j = bisect.bisect(R, entry)

        # Boundary cases
        if j == 0:
            return 0
        if j == len(R):
            return len(R) - 1

        # General case:  Find the closer candidate
        assert time >= R[j - 1].walltime
        assert time <= R[j].walltime
        if time - R[j - 1].walltime < R[j].walltime - time:
            return j - 1
        return j

    def _get_dummy_metrics(self):
        """Default dummy metrics.
        
        These are used if we have no data, e.g. for survey type 0.
        We put in some usage numbers which are good defaults from experience.
        """
        # XXX:  These should depend on the channel used:  RX and TX should be
        # 0 if it's a different channel than the AP was on in the time slice
        # requested.
        return ChannelMetrics(
                        0,                          # time delta
                        False,                      # is_valid
                        0,                          # measurement time
                        0.2,                        # busy fraction
                        0.02,                       # RX fraction
                        0.05)                       # TX fraction

    def get_timeslice_metrics(self, time_begin, time_end):
        """Get metrics of the time span between idx_begin and idx_end.

        @return ChannelMetrics object."""
        assert time_end >= time_begin
        R = self.stats_rows

        if self.survey_type == 0 or len(self.stats_rows) == 0:
            return self._get_dummy_metrics()
        elif self.survey_type == 1:
            # survey_type 1:  this is the full accumulation survey type.
            idx_beg = self.find_index_for_time(time_begin)
            idx_end = self.find_index_for_time(time_end)

            # Adjust indices to fit into the array
            if idx_end == len(R):
                idx_end -= 1
                if idx_beg == len(R):
                    idx_beg -= 2
                if idx_beg < 0:
                    idx_beg = 0
            
            # If indices are the same, we need to return an approximation.
            # We try to estimate from the average for this channel, if we can.
            if idx_beg == idx_end:
                ret = self._get_dummy_metrics()

                t = R[idx_beg].time
                if t is not None and t > 0:
                    ret.delta_t = t
                    if R[idx_beg].busy is not None:
                        ret.f_busy = R[idx_beg].busy / t
                    if R[idx_beg].rx is not None:
                        ret.f_rx = R[idx_beg].rx / t
                    if R[idx_beg].tx is not None:
                        ret.f_tx = R[idx_beg].tx / t
                return ret

            # Otherwise we may have some real stats.  Cool.
            def _diff(key, idx_beg, idx_end):
                if R[idx_end].__dict__[key] is not None and  \
                  R[idx_beg].__dict__[key] is not None:
                    return R[idx_end] - R[idx_beg]
                return None
            d = _diff("time", idx_beg, idx_end)
            if d is None:
                return self._get_dummy_metrics()
            d_busy = _diff("busy", idx_beg, idx_end)
            d_rx = _diff("rx", idx_beg, idx_end)
            d_tx = _diff("tx", idx_beg, idx_end)
            def _frac(num, denom, default):
                if num is None:
                    return default
                return num / denom
            is_valid = d_busy is not None and \
                       d_rx is not None and \
                       d_tx is not None
            return ChannelMetrics(R[idx_end].walltime - R[idx_beg].walltime,
                                is_valid,
                                d,
                                _frac(d_busy, d),
                                _frac(d_rx, d),
                                _frac(d_tx, d))
        elif self.survey_type == 2:
            # This is the local sampling survey type
            idx_beg = self.find_index_for_time(time_begin)
            idx_end = self.find_index_for_time(time_end)
            if idx_end == len(R):
                idx_end -= 1
            
            d = 0
            busy_n, busy_d = 0, 0
            rx_n, rx_d = 0, 0
            tx_n, tx_d = 0, 0
            for i in range(idx_beg, idx_end + 1):
                if R[i].time is None:
                    continue
                d += R[i].time
                if R[i].busy is not None:
                    busy_n += R[i].busy
                    busy_d += R[i].time
                if R[i].rx is not None:
                    rx_n += R[i].rx
                    rx_d += R[i].time
                if R[i].tx is not None:
                    tx_n += R[i].tx
                    tx_d += R[i].time
            def _frac(x, y, default):
                if y == 0:
                    return default
                return x/y
            # XXX: values taken from dummy metrics, should depend on whether
            # channel was in use.
            busy_f = _frac(busy_n, busy_d, 0.2)
            rx_f = _frac(rx_n, rx_d, 0.02)
            tx_f = _frac(tx_n, tx_d, 0.05)

            # Channel metrics to return
            ret = ChannelMetrics(
                    R[idx_end].walltime - R[idx_beg].walltime,
                    True,
                    d,
                    busy_f,
                    rx_f,
                    tx_f)
            return ret

    def propose_time_boundaries(self, delta):
        R = self.stats_rows
        if len(R) == 0:
            return []
        boundaries = [ R[0].walltime ]
        for ent in R:
            if ent.walltime - boundaries[-1] >= delta:
                boundaries.append(ent.walltime)
        return boundaries


def get_node_channel_scores(conn, cursor, nodes, node_id,
  neighbors, channel_freqs, epoch_start = None):
    """Get the channel baseline scores.

    @param  conn, cursor
            database connection and cursor.

    @param  nodes
            array of nodes.

    @param  node_id
            index of the radio to investigate
            (index of the nodes in the array).

    @param  neighbors
            list of neighbor nodes (indices into nodes array).

    @param  channel_freqs
            list of channels inspect, given by freqency.

    @param  epoch_start
            the start of the new epoch, if any.
    """

    #  The cutoff is how far into the past we want to dig.
    time_cutoff = int(time.time()) - hist_size * 86400
    if epoch_start is not None:
        time_cutoff = max(epoch_start, time_cutoff)

    ret = []
    for freq in channel_freqs:
        # Get the node statistics
        node_stats = ChannelStatsRows(conn, cursor,
                        nodes[node_id][0], freq, time_cutoff)

        # Compile the section scores
        # (Those are scores for the individual time slots.)
        #
        # Steps:
        # (1) baseline value from this node
        # (2) correction from the corresponding TX values of the neighbors
        #     we have control over

        # Time boundaries
        bound = node_stats.propose_time_boundaries(300)

        # Get the baseline values
        section_scores = []
        for i in range(len(bound) - 1):
            M = node_stats.get_timeslice_metrics(bound[i], bound[i + 1])
            section_scores.append( M.f_busy - M.f_tx )

        # Now subtract compensations from each of the controlled neighbors
        for neigh_id in neighbors:
            neigh_stats = ChannelStatsRows(conn, cursor,
                                nodes[neigh_id][0], freq, time_cutoff)

            for i in range(len(bound) - 1):
                M = neigh_stats.get_timeslice_metrics(bound[i], bound[i + 1])
                section_scores[i] -= M.f_tx

        # From the section stats, extract a good value
        score = None
        if len(section_scores) == 0:
            # No data?  We return a very optimistic score, so that this
            # channel is likely to be explored.
            score = 0
        else:
            # Choose a channel with fairly high load; we take the
            # 7/8-th quantile
            score = sorted(section_scores)[len(section_scores) * 7 // 8]
        ret.append(score)
    return ret

def get_tx_value(conn, cursor, nodes, node_id, epoch_start = None):
    """For a node, return its TX fraction."""

    survey_type = get_survey_type(conn, cursor, nodes[node_id][0])
    if survey_type == 0:
        # We don't have survey data; just make a guess.
        # XXX
        return 0.05

    # Get the slices of the TX value
    time_cutoff = int(time.time()) - hist_size  * 86400
    if epoch_start is not None:
        time_cutoff = max(epoch_start, time_cutoff)
    sql = """SELECT servertime, time, busy, rx, tx, frequency
    FROM wrinfo, wrinfo_survey
    WHERE wrinfo.wrinfo_id = wrinfo_survey.wrinfo_id
    AND in_use = 1
    AND radio_if_id = %d
    AND servertime >= %d
    ORDER BY wrinfo.wrinfo_id;""" \
      % (nodes[node_id][0], time_cutoff)
    cursor.execute(sql)
    stats = list(cursor)

    # Evaluate slices
    # A TX value is computed for each slice and put into the v[] array.
    v = []
    if survey_type == 1:
        for i in range(len(stats) - 1):
            # Check if data is available and if it is sane
            if stats[i][4] is None or stats[i + 1][4] is None:
                continue
            if stats[i][1] is None or stats[i + 1][1] is None:
                continue
            if stats[i + 1][5] != stats[i][5]:
                # Frequency changed, can't use data here.
                continue
            if stats[i + 1][1] <= stats[i][1]:
                continue
            v.append((stats[i + 1][4] - stats[i][4])
                / float(stats[i + 1][1] - stats[i][1]))
    elif survey_type == 2:
        for i in range(len(stats)):
            if stats[i][4] is None or stats[i][1] is None:
                continue
            if stats[i][1] == 0:
                continue
            v.append(stats[i][4] / stats[i][1])

    # Compute the statistic to return
    #
    # Right now we use the 7/8-th percentile with the hope to capture a
    # somewhat large TX value;  the idea is that channel selection matters
    # at those times when there is load on the network, not when there is
    # none.
    if len(v) == 0:
        return 0
    return sorted(v)[len(v) * 7 // 8]

def _usage():
    print("Data collector.  Extracts data from DB and postprocesses it")
    print("")
    print("   -h        help")
    print("   -d <n>    database name")
    print("   -c #      channel frequencies to consider, comma separated")
    print("   -e #      epoch start, if any (disregard data before that point)")
    print("   -r #      set history size to the given number of days")

if __name__ == "__main__":
    # Default settings
    dbname = 'wifispecman'
    channels = [ 5180, 5200, 5220 ]
    epoch_start = None

    # Read options
    import sys
    import getopt
    opts, args = getopt.getopt(sys.argv[1:], 'hd:c:e:r:')
    for k, v in opts:
        if k == '-h':
            _usage()
            sys.exit(1)
        elif k == '-d':
            dname = v
        elif k == '-c':
            channels = [ int(x) for x in v.split(",") ]
        elif k == '-e':
            epoch_start = int(v)
        elif k == '-r':
            hist_size = float(v)

    # Open DB
    conn, cursor = open_db(dbname)

    nodes = get_nodes(conn, cursor)
    print("Nodes:", nodes)

    graph = get_graph(conn, cursor, nodes)
    print("Graph:", graph)

    # channel scores
    scores = []
    for i in range(len(nodes)):
        sc_this_if = get_node_channel_scores(conn, cursor, nodes,
                    i,
                    graph.get(i, []),
                    channels,
                    epoch_start)
        print("Channel scores for node", i, ":", sc_this_if)
        scores.append(sc_this_if)

    # channel TX values
    tx_values = []
    for i in range(len(nodes)):
        tx = get_tx_value(conn, cursor, nodes, i, epoch_start)
        print("TX value for node", i, ":", tx)
        tx_values.append(tx)

    conn.close()

# vim:sw=4:sts=4:et
