#include <assert.h>
#include <errno.h>
#include <stdio.h>

#include "nl_inc.h"
#include "nlcctx.h"

struct nlcctx* nlcctx_create(const char* if_name)
{
	struct nlcctx* ret = NULL;
	struct nl_sock* sock = NULL;

	/* Create the socket */
	sock = nl_socket_alloc();
	if (sock == NULL) {
		fprintf(stderr, "Error:  nl_socket_alloc() failed.\n");
		goto xit;
	}

	if (genl_connect(sock) != 0) {
		fprintf(stderr, "Error:  genl_connect() failed.\n");
		goto xit;
	}

	/* Try to set NETLINK_EXT_ACK socket option, ignoring errors */
	int err = 1;
	setsockopt(nl_socket_get_fd(sock),
		SOL_NETLINK,
		NETLINK_EXT_ACK,
		&err,
		sizeof(err));

	/* Add to "scan" group
	 * This is needed to perform network scans.
	 */
	const int mcid = genl_ctrl_resolve_grp(sock, "nl80211", "scan");
	if (mcid < 0) {
		fprintf(stderr, "Error:  Can't resolve multicast group "
		  "`scan'\n");
		goto xit;
	}
	err = nl_socket_add_membership(sock, mcid);
	if (err < 0) {
		fprintf(stderr, "Error:  nl_socket_add_membership() failed "
		  "for group `scan'\n");
		goto xit;
	}

	/* Install verbose callbacks so errors are logged */
	struct nl_cb* cb = nl_cb_alloc(NL_CB_VERBOSE);
	if (cb == NULL) {
		fprintf(stderr, "Error:  nl_cb_alloc() failed\n");
		goto xit;
	}
	nl_socket_set_cb(sock, cb);
	nl_cb_put(cb);

	/* Disable sequence checks since we're also listening for
	 * multicast messages
	 */
	nl_socket_disable_seq_check(sock);

	/* Find the "nl80211" family ID */
	int family = genl_ctrl_resolve(sock, "nl80211");
	if (family < 0) {
		fprintf(stderr, "Error: genl_ctrl_resolve() failed\n");
		goto xit;
	}

	/* Get the device */
	const int interf = if_nametoindex(if_name);
	if (interf == 0) {
		fprintf(stderr, "Error:  if_nametoindex failed: %s\n",
		  strerror(errno));
		goto xit;
	}

	/* Create the context */
	ret = malloc(sizeof(struct nlcctx));
	if (ret == NULL) {
		perror("malloc");
		goto xit;
	}
	ret->sock = sock;
	ret->family = family;
	ret->interf = interf;
xit:
	if (ret == NULL) {
		/* Failure case */
		if (sock != NULL)
			nl_socket_free(sock);
		return NULL;
	}

	/* Success */
	return ret;
}

void nlcctx_free(struct nlcctx* ctx)
{
	assert(ctx != NULL);
	nl_socket_free(ctx->sock);
	free(ctx);
}

int nlcctx_msg_exchange(struct nlcctx* ctx,
			int nl_cmd_id,
			int (*cb)(struct nl_msg* msg, void* uptr),
			void* uptr)
{
	struct nl_msg *msg = nlmsg_alloc();
	genlmsg_put(msg,			// Message
		NL_AUTO_PORT,			// Port
		NL_AUTO_SEQ,			// Sequence number
		ctx->family,			// Family identifier
		0,				// Header Length
		NLM_F_DUMP,			// Flags
		nl_cmd_id,			// Command ID
		0);				// Interface version
	nla_put_u32(msg, NL80211_ATTR_IFINDEX, ctx->interf);
	nl_socket_modify_cb(ctx->sock, NL_CB_VALID, NL_CB_CUSTOM, cb, uptr);

	int ret = nl_send_auto(ctx->sock, msg);
	nlmsg_free(msg);

	ret = nl_recvmsgs_default(ctx->sock);
	if (ret < 0) {
		fprintf(stderr, "Error: nl_recvmsgs_default() error: %s (%d).\n",
				nl_geterror(-ret), ret);
		return ret;
	}

	return 0;
}
