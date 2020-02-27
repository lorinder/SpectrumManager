#include <sys/socket.h>

#include <net/if.h>

#include <linux/netlink.h>
#include <linux/nl80211.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

/* Compatibility to libnl-tiny */
#ifndef NL_AUTO_PORT
#  define NL_AUTO_PORT	NL_AUTO_PID
#  define nl_send_auto	nl_send_auto_complete
#endif
