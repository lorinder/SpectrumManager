#!/bin/bash

DIR=~



echo 1 4 5 \
| xargs -n 1 -P 8 ${DIR}/run_iperf.sh 


