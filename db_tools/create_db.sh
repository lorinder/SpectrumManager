#!/bin/sh

dbname=$1
if [ -z "$dbname" ] ; then
	dbname=wifispecman
	msg="Using default data base name wifispecman since none given."
	echo "Info: ${msg}" 1>&2
fi

mysql -u root <<EOF
-- script to create the main database
--
-- creates the user, database, and empty tables. 
--
-- needs to be run as the mysql root user.
CREATE USER 'wifispecman'@'localhost' IDENTIFIED BY 'password' ACCOUNT UNLOCK;
CREATE DATABASE ${dbname};
GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, INDEX, ALTER,
      CREATE TEMPORARY TABLES ON ${dbname}.* 
      TO 'wifispecman'@'localhost'
      IDENTIFIED BY 'password';
FLUSH privileges;
EOF
