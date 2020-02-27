#include <assert.h>
#include <stdbool.h>

#include "jdump.h"
#include "jdattr.h"

#include "nl_inc.h"
#include "nlcctx.h"

#include "wifi_survey.h"

static int cb_dump(struct nl_msg* msg, void* arg)
{
	struct genlmsghdr* gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr* tb[NL80211_ATTR_MAX + 1];
	struct nlattr* survey[NL80211_SURVEY_INFO_MAX + 1];
	static struct nla_policy survey_info_policy[NL80211_SURVEY_INFO_MAX + 1] = {
		[NL80211_SURVEY_INFO_FREQUENCY] = { .type = NLA_U32 },
		[NL80211_SURVEY_INFO_IN_USE] = { .type = NLA_FLAG },
		[NL80211_SURVEY_INFO_NOISE] = { .type = NLA_U8 },
		[NL80211_SURVEY_INFO_TIME] = { .type = NLA_U64 },
		[NL80211_SURVEY_INFO_TIME_BUSY] = { .type = NLA_U64 },
		[NL80211_SURVEY_INFO_TIME_EXT_BUSY] = { .type = NLA_U64 },
		[NL80211_SURVEY_INFO_TIME_RX] = { .type = NLA_U64 },
		[NL80211_SURVEY_INFO_TIME_TX] = { .type = NLA_U64 },
	};

	/* Parse */
	nla_parse(tb,				// Array to be filled
		NL80211_ATTR_MAX,		// Maximum array type to be expected
		genlmsg_attrdata(gnlh, 0),	// Head of attribute stream to parse
		genlmsg_attrlen(gnlh, 0),	// Length of attribute stream
		NULL);				// Validation policy
	if (!tb[NL80211_ATTR_SURVEY_INFO]) {
		fprintf(stderr, "Error:  survey_info attribute info missing!\n");
		return NL_SKIP;
	}
	if (nla_parse_nested(survey,
			NL80211_SURVEY_INFO_MAX,
			tb[NL80211_ATTR_SURVEY_INFO],
			survey_info_policy))
	{
		fprintf(stderr, "Error:  Failed to parse nested survey_info attribute!\n");
		return NL_SKIP;
	}

	/* Output into JSON */
	jdump_state* jd = (jdump_state*)arg;
	jdump_put_object(jd);
	jdattr_u32(jd, "frequency", survey[NL80211_SURVEY_INFO_FREQUENCY]);
	jdattr_flag(jd, "in_use", survey[NL80211_SURVEY_INFO_IN_USE]);
	jdattr_i8(jd, "noise", survey[NL80211_SURVEY_INFO_NOISE]);
	jdattr_u64(jd, "time", survey[NL80211_SURVEY_INFO_TIME]);
	jdattr_u64(jd, "busy", survey[NL80211_SURVEY_INFO_TIME_BUSY]);
	jdattr_u64(jd, "ext_busy", survey[NL80211_SURVEY_INFO_TIME_EXT_BUSY]);
	jdattr_u64(jd, "rx", survey[NL80211_SURVEY_INFO_TIME_RX]);
	jdattr_u64(jd, "tx", survey[NL80211_SURVEY_INFO_TIME_TX]);
	jdump_close_object(jd);

	return NL_SKIP;
}

int wifi_get_survey_results(struct nlcctx* ctx, jdump_state* jd)
{
	jdump_put_array(jd);
	const int ret = nlcctx_msg_exchange(ctx, NL80211_CMD_GET_SURVEY, cb_dump, jd);
	jdump_close_array(jd);
	return ret;
}
