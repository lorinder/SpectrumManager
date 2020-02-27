SpectrumManager software
========================

Overview
--------

The SpectrumManager software improves quality of WiFi networks.  Given a
set of controlled APs (access points) that interfere with each other and
also other networks, it takes statistics on the WiFi channels of each AP
and then finds a channel assignment such that interference in the system
is minimized.  For more details, please look at the slide deck:

[Overview Presentation](https://docs.google.com/presentation/d/1rZx-AdGONEt-BR_LS4VZvnRWa1NllXvYzsO8W6eijoQ/edit?usp=sharing)

Setup instructions
------------------

### Database

On a central server, install mariadb and set up the database with
`db_tools/create_db.sh`.  Populate the database with informations about
their APs and network interfaces.  The `sample_ap_insert.sql` gives an
example for a single AP.

### Prepare APs

Add the server SSH key to each AP so that it has passwordless login.
Compile `wrinfo` for the APs (presumably using an OpenWRT SDK), and copy
it to each AP's root home directory.

Also, copy the `optimizer/setchan.sh` script onto the APs;  this script
will be used to switch channels on the fly.

### Cronjobs

On the server system, install cronjobs to execute `run_wrinfo.py`
periodically, so that `wrinfo` is run on each AP periodically, e.g.,
every few minutes.  Less frequently (since it is more invasive),
`wrinfo` should also be run with the `-s` flag to tell it to perform a
channel scan.  Take a look at `sample_cron_scripts/wrinfo_periodic.sh`
for an example cron script.  Also look at the `sample_crontab.txt` in
the same directory.

Also, run the channel optimizer periodically, e.g. once an hour or once
a day.  See the `sample_cron_scripts/run_optimizer.sh` script for an
example.

### Generating Network Load

For testing, it may make sense to create artificial traffic on the
network; the `run_iperf.sh` and `run_all_iperfs.sh` was what we used for
that purpose.

Status
------

This current implementation is a proof of concept that has been verified
on a small system with 3 APs and 15 connected clients.  The basic design
is workable, but the implementation is not production ready at this
point.
