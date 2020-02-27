#include <assert.h>
#include <stdbool.h>
#include <inttypes.h>

#include "jdump.h"
#include "jdattr.h"

#include "nl_inc.h"
#include "nlcctx.h"

#include "wifi_interface.h"

static int cb_dump(struct nl_msg* msg, void* arg)
{
	struct genlmsghdr* gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr* tb[NL80211_ATTR_MAX + 1];

	/* Parse */
	nla_parse(tb,				// Array to be filled
		NL80211_ATTR_MAX,		// Maximum array type to be expected
		genlmsg_attrdata(gnlh, 0),	// Head of attribute stream to parse
		genlmsg_attrlen(gnlh, 0),	// Length of attribute stream
		NULL);				// Validation policy

	/* Output into JSON */
	jdump_state* jd = (jdump_state*)arg;
	jdump_put_object(jd);
	jdattr_str(jd, "interface", tb[NL80211_ATTR_IFNAME]);
	jdattr_u32(jd, "ifindex", tb[NL80211_ATTR_IFINDEX]);
	jdattr_macaddr(jd, "mac", tb[NL80211_ATTR_MAC]);
	jdattr_str(jd, "ssid", tb[NL80211_ATTR_SSID]);
	jdattr_u32(jd, "frequency", tb[NL80211_ATTR_WIPHY_FREQ]);
	jdattr_u32(jd, "channel_width_enum", tb[NL80211_ATTR_CHANNEL_WIDTH]);
	jdattr_u32(jd, "center_freq1", tb[NL80211_ATTR_CENTER_FREQ1]);
	jdattr_u32(jd, "center_freq2", tb[NL80211_ATTR_CENTER_FREQ2]);
	jdattr_u32(jd, "channel_type_enum", tb[NL80211_ATTR_WIPHY_CHANNEL_TYPE]);
	jdump_close_object(jd);

	return NL_SKIP;
}

int wifi_get_interface_info(struct nlcctx* ctx, jdump_state* jd)
{
	return nlcctx_msg_exchange(ctx, NL80211_CMD_GET_INTERFACE, cb_dump, jd);
}
