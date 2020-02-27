Tools to create, manage and update the Wifi statistics database
---------------------------------------------------------------

The database uses MariaDB.  So the corresponding database server
needs to be running on the machine in question.

Tools
-----

* db creation.
  This consists of three steps:
    (1) Create the database and its user.
        Run ./create_db.sh (needs to be run as root user.)
    (2) Create the tables in the database.
        ./create_tables.sh
    (3) Add the APs and their relevant radio interfaces to the database.
        The sample_ap_insert.sql script gives an example of an AP
        which is at 192.168.1.1, and having a relevant interface "wlan1".

* db deletion.

	sudo mariadb -u root < delete_db.sql

* obtaining scan results and inserting into DB.

  The run_wrinfo.py runs the wrinfo script remotely to obtain results on
  of the interfaces and updates the database with the corresponding
  results.  The -i switch specifies the ID of the interface; the ID here is
  the primary key of the radio_if table in the database.  Example:

  	./run_wrinfo.py -i 1

* Running wrinfo for all the interfaces.

  Presumably one would want to automatically query all the active
  interfaces regularly, so doing the above by hand is not practical.
  The details on how this would be automated depends on the site setup,
  e.g. cron could be used, or some other scheduler, to automatically run
  this on all the interfaces.  One might prefer a staggered or
  continuous approach in some scenarios.

  As a starting point, an example to run run_wrinfo for all the
  interface, here is a simple shell snippet to do that:

	echo "select id from radio_if;" \
	  | mariadb -B -N -u wifispecman --password=password wifispecman \
	  | xargs -n 1 -P 8 ./run_wrinfo.py -i

  This version runs 8 copies of run_wrinfo.py in parallel.
 
Database documentation
----------------------

The tables of the DB are as follows.

### ap--listing of all the APs

- id	 	primary key
- ip_addr	IP address of AP as x.y.z.w string
- in_use	Set to "false" to temporarily disable processing this AP.

### radio_if--Listing of all the network interfaces

- id		primary key
- ap_id	 	AP this interface belongs to.
- ifname	OS name of interface, e.g. "wlan0"
- measuring	1 if we're scanning this interface
- optimising	1 if we're optimizing channels for this interface
- wrinfo_cmd	if not NULL, the shell command to run wrinfo; otherwise
		it's constructed automatically by run_wrinfo.  Should be
		NULL other than for testing.
- survey_type	type of survey produced by this AP
		0-no useful survey
		1-full accumulation; survey counters are monotonically
		  increasing
		2-short-term resets; survey counters reset automatically every
		  few seconds and are thus

### rungroup--Listing of all the run_wrinfo runs

- rungroup_id	unique ID
- cmd	 	the run_wrinfo command associated with this rungroup
- servertime	Wall time when this was initiated
- tag	 	arbitrary tag specified on run_wrinfo command line when running

### wrinfo--Listing of all wrinfo runs

- wrinfo_id	unique ID
- radio_if_id	radio interface that was queried
- status	0: ok, 1: nonzero exit code, 2: timeout, 3: invalid data
- exit_code	wrinfo/ssh process exit status
- servertime	Unix timestamp of attempt
- rungroup_id	Corresponding rungroup

### wrinfo_meta--Listing of metadata wrinfo output

- wrinfo_id
- cmd	 	Command line of wrinfo
- time_human	AP time, human readable
- time_unix	AP time, unix timestamp

### wrinfo_interface--Listing of interface wrinfo output

- wrinfo_id 
- ifindex	Numeric IF index in AP
- mac		IF MAC address
- ssid		Service Set ID
- frequency	Operating Frequency [MHz]
- channel_width_enum 
- center_freq1 
- center_freq2 
- channel_type_enum 

### wrinfo_scan--Listing of scan wrinfo outputs (channel scan)

- id
- wrinfo_id	wrinfo run ID
- bssid	 	BSSDID found
- frequency	Frequency it was found at
- ssid	 	Corresponding SSID

### wrinfo_survey--Listing of survey wrinfo outputs

- id
- wrinfo_id
- frequency	Channel Frequency
- in_use	True if IF is using this channel
- noise		Noise level of channel
- time		Amount of time channel was active
- busy		Amount of time channel was busy
- rx		Amount of RX time on channel
- tx		Amount of TX time on channel

### wrinfo_errors--Listing of wrinfo error messages

This is simply a verbatim dump of stderr.

- id
- wrinfo_id
- msg		One line of an error message
