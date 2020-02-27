#!/bin/bash

# run iperf on the AP specified by the integer

if [ -z "$1" ] ; then
	echo "please specify ap number on command line." 1>&2
	exit 1
fi

time=60

ip=192.168.4.$((20 + $1))

run_iperf() {
	ssh root@$ip "./iperf_parallel.sh 5"
}

echo "$1 $ip `run_iperf`"
