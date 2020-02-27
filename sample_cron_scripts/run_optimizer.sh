#!/bin/sh

DIR=/home/wifispecman/prj/optimizer

{ date & ${DIR}/optimizer.py -s;} >> ${DIR}/log.txt

