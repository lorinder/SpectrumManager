#ifndef NLCCTX_H
#define NLCCTX_H

/**	@file	nlcctx.h
 *
 *	Netlink communications context.  Contains all the information
 *	necessary to communicate over netlink; this includes the socket
 *	itself, family ID, interface index, and so on.
 */

struct nl_sock;
struct nl_msg;

struct nlcctx {
	struct nl_sock* sock;		// The netlink socket
	int family;			// The ID of the "nl80211" family
	int interf;			// The index of the network interface
};

struct nlcctx* nlcctx_create(const char* if_name);
void nlcctx_free(struct nlcctx* ctx);

int nlcctx_msg_exchange(struct nlcctx* ctx,
				int nl_cmd_id,
				int (*cb)(struct nl_msg* msg, void* uptr),
				void* arg);

#endif /* NLCCTX_H */
