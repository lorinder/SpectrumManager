#!/bin/sh

#  Instruct the AP to switch frequency.  First argument is the interface
#  (e.g. "wlan0"), second one is the new channel frequency ("2412")

echo "chan_switch 5 $2" | hostapd_cli -i "$1"
