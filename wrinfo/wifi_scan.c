#include <ctype.h>
#include <stdbool.h>
#include <inttypes.h>

#include "jdump.h"
#include "jdattr.h"

#include "nl_inc.h"
#include "nlcctx.h"

#include "wifi_scan.h"

static int cb_scan(struct nl_msg *msg, void *uptr)
{
	bool* pcomplete = uptr;

	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	switch (gnlh->cmd) {
	case NL80211_CMD_SCAN_ABORTED:
		fprintf(stderr, "Note: Got NL80211_CMD_SCAN_ABORTED; scan was "
		  "aborted\n");
		/* fall through */
	case NL80211_CMD_NEW_SCAN_RESULTS:
		*pcomplete = true;
		break;
	default:
		/* Since we're listening to `scan' multicast message, we
		 * might also receive other messages; we ignore those.
		 */
		;
	}

	return NL_OK;
}

int wifi_scan(struct nlcctx* ctx)
{
	bool success = true;
	bool scan_complete = false;

	/* Allocate the message */
	struct nl_msg* msg = nlmsg_alloc();
	struct nl_msg* ssids_to_scan = nlmsg_alloc();
	if (!msg || !ssids_to_scan) {
		fprintf(stderr, "Error:  nlmsg_alloc() failed\n");
		success = false;
		goto cleanup;
	}

	/* Prepare the scan trigger command */
	genlmsg_put(msg,			// Message
		NL_AUTO_PORT,			// Port
		NL_AUTO_SEQ,			// Sequence number
		ctx->family,			// Family identifier
		0,				// Header Length
		0,				// Flags
		NL80211_CMD_TRIGGER_SCAN,	// Command ID
		0);				// Interface version
	nla_put_u32(msg, NL80211_ATTR_IFINDEX, ctx->interf);
	nla_put(ssids_to_scan, 1, 0, "");
	nla_put_nested(msg, NL80211_ATTR_SCAN_SSIDS, ssids_to_scan);

	/* Add the callback */
	nl_socket_modify_cb(ctx->sock,		// Socket
				NL_CB_VALID,
				NL_CB_CUSTOM,
				cb_scan,	// callback
				&scan_complete);// user pointer

	/* Send the trigger command */
	int err = nl_send_auto(ctx->sock, msg);
	if (err < 0) {
		fprintf(stderr, "Error:  nl_send_auto() failed: %s\n",
		  nl_geterror(-err));
		success = false;
		goto cleanup;
	}

	/* Wait for scan to complete or abort */
	do {
		err = nl_recvmsgs_default(ctx->sock);
	} while (err >= 0 && !scan_complete);

	if (err < 0) {
		fprintf(stderr, "Error:  nl_recvmsgs_default failed: %s\n",
		  nl_geterror(-err));
		success = false;
		goto cleanup;
	}

	/* Cleanup */
cleanup:
	if (ssids_to_scan)
		nlmsg_free(ssids_to_scan);
	if (msg)
		nlmsg_free(msg);
	return (success ? 0 : -1);
}

/* Implementation of wifi_get_scan_results */

/* Extract the SSID from the information elements and prints it */
static void dump_ssid(jdump_state* jd, unsigned char *ie, int ielen)
{
	while (ielen >= 2 && ielen >= ie[1] + 2) {
		const int len = ie[1];
		if (ie[0] == 0)
		{
			/* Information element with ID 0 is the SSID;
			 * print it and terminate loop.
			 */
			jdump_put_pod(jd, (char*)ie + 2, len, true);
			break;
		}

		/* Move to next IE */
		ielen -= len + 2;
		ie += len + 2;
	}
}

static int cb_dump(struct nl_msg *msg, void *uptr)
{
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct nlattr *bss[NL80211_BSS_MAX + 1];
	static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
		[NL80211_BSS_TSF] = { .type = NLA_U64 },
		[NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
		[NL80211_BSS_BSSID] = { .minlen = 6 },
		[NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
		[NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
		[NL80211_BSS_INFORMATION_ELEMENTS] = { },
		[NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
		[NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
		[NL80211_BSS_STATUS] = { .type = NLA_U32 },
		[NL80211_BSS_SEEN_MS_AGO] = { .type = NLA_U32 },
		[NL80211_BSS_BEACON_IES] = { },
	};

	/* Parse msg, in particular BSS nexted attribute */
	nla_parse(tb,				// Array to be filled
		NL80211_ATTR_MAX,		// Maximum array type to be expected
		genlmsg_attrdata(gnlh, 0),	// Head of attribute stream to parse
		genlmsg_attrlen(gnlh, 0),	// Length of attribute stream
		NULL);				// Validation policy
	if (!tb[NL80211_ATTR_BSS]) {
		fprintf(stderr, "Error:  BSS info missing.\n");
		return NL_SKIP;
	}
	if (nla_parse_nested(bss, NL80211_BSS_MAX, tb[NL80211_ATTR_BSS], bss_policy)) {
		fprintf(stderr, "Error:  Could not parse BSS attribute.\n");
		return NL_SKIP;
	}

	/* Print output
	 * We wrap everyting into a json element.
	 */
	jdump_state* jd = (jdump_state*)uptr;
	jdump_put_object(jd);
	jdattr_macaddr(jd, "bssid", bss[NL80211_BSS_BSSID]);
	jdattr_u32(jd, "frequency", bss[NL80211_BSS_FREQUENCY]);
	if (bss[NL80211_BSS_INFORMATION_ELEMENTS]) {
		jdump_put_key(jd, "ssid");
		dump_ssid(jd,
			   nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]),
		           nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]));
	}
	jdump_close_object(jd);

	return NL_SKIP;
}

int wifi_get_scan_results(struct nlcctx* ctx, jdump_state* jd)
{
	jdump_put_array(jd);
	const int ret = nlcctx_msg_exchange(ctx, NL80211_CMD_GET_SCAN, cb_dump, jd);
	jdump_close_array(jd);

	return ret;
}
