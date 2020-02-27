#!/bin/bash

echo "select id from radio_if;" \
  | mariadb -B -N -u wifispecman --password=password wifispecman \
  | xargs -n 1 -P 8 $HOME/prj/db_tools/run_wrinfo.py "$@" -i
