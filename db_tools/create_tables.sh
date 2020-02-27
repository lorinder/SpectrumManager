#!/bin/sh

dbname=$1
if [ -z "$dbname" ] ; then
	dbname=wifispecman
	msg="Using default data base name wifispecman since none given."
	echo "Info: ${msg}" 1>&2
fi

mysql -u wifispecman --password=password ${dbname} <<EOF
CREATE TABLE IF NOT EXISTS ap
	(id INT AUTO_INCREMENT UNIQUE PRIMARY KEY,
	ip_addr VARCHAR(16),
	in_use BOOLEAN);

CREATE TABLE IF NOT EXISTS radio_if
	(id INT AUTO_INCREMENT UNIQUE PRIMARY KEY,
	ap_id INT,
	ifname VARCHAR(8),
	measuring INT,
	optimising INT,
	wrinfo_cmd VARCHAR(128),

	survey_type INT);



CREATE TABLE IF NOT EXISTS rungroup
	(rungroup_id INT AUTO_INCREMENT UNIQUE PRIMARY KEY,
	cmd VARCHAR(128),
	servertime BIGINT,
	tag VARCHAR(24));


CREATE TABLE IF NOT EXISTS wrinfo
	(wrinfo_id INT AUTO_INCREMENT UNIQUE PRIMARY KEY,
	radio_if_id INT,
	status INT,

	exit_code INT,
	servertime BIGINT,
	rungroup_id INT);


CREATE TABLE IF NOT EXISTS wrinfo_meta
	(wrinfo_id INT UNIQUE PRIMARY KEY,
	cmd VARCHAR(256),
	time_human VARCHAR(40),
	time_unix BIGINT);


CREATE TABLE IF NOT EXISTS wrinfo_interface
	(wrinfo_id INT UNIQUE PRIMARY KEY,
	ifindex INT,
	mac VARCHAR(18),
	ssid VARCHAR(32),
	frequency INT,
	channel_width_enum SMALLINT,
	center_freq1 INT,
	center_freq2 INT,
	channel_type_enum SMALLINT);


CREATE TABLE IF NOT EXISTS wrinfo_scan
	(id INT AUTO_INCREMENT UNIQUE PRIMARY KEY,
	wrinfo_id INT,
	bssid VARCHAR(18),
	frequency INT,
	ssid VARCHAR(32));


CREATE TABLE IF NOT EXISTS wrinfo_survey
	(id INT AUTO_INCREMENT UNIQUE PRIMARY KEY,
	wrinfo_id INT,
	frequency INT,
	in_use BOOLEAN,
	noise INT,
	time BIGINT,
	busy BIGINT,
	rx BIGINT,
	tx BIGINT);



CREATE TABLE IF NOT EXISTS wrinfo_errors
	(id INT AUTO_INCREMENT UNIQUE PRIMARY KEY,
	wrinfo_id INT,
	msg VARCHAR(256));
EOF
