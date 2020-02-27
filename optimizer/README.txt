Channel optimization implementation
===================================

(1) setchan.sh needs to be on the APs.  This is the one-liner shell
    script used to actually switch channels.

(2) Running "./optimizer.py -s" will use the wifispecman DB to retrieve
    the data and find a good assignment.  Channel switching will then be
    performed.  If the -s flag is omitted, the good assignment will be
    computed but not executed.
