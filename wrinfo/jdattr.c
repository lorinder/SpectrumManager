#include <stdio.h>
#include <inttypes.h>

#include "jdump.h"
#include "nl_inc.h"

void jdattr_flag(jdump_state* jd, const char* name, struct nlattr* attr)
{
	jdump_put_key(jd, name);
	jdump_put_pod(jd, attr ? "true" : "false", -1, false);
}

#define jdattr_int(jd, name, attr, cast, fmt, getter_suffix) \
	do { \
		if (attr) { \
			jdump_put_key(jd, name); \
			jdump_put_pod(jd, NULL, -1, false); \
			printf("%" fmt, \
			  (cast)nla_get_ ## getter_suffix(attr)); \
		} \
	} while(0)

void jdattr_i8(jdump_state* jd, const char* name, struct nlattr* attr)
{
	jdattr_int(jd, name, attr, int8_t, PRIi8, u8);
}

void jdattr_u32(jdump_state* jd, const char* name, struct nlattr* attr)
{
	jdattr_int(jd, name, attr, uint32_t, PRIu32, u32);
}

void jdattr_u64(jdump_state* jd, const char* name, struct nlattr* attr)
{
	jdattr_int(jd, name, attr, uint64_t, PRIu64, u64);
}

#undef jdattr_int

void jdattr_str(jdump_state* jd, const char* name, struct nlattr* attr)
{
	if (attr) {
		jdump_put_key(jd, name);
		jdump_put_pod(jd, nla_get_string(attr), -1, true);
	}
}

void jdattr_macaddr(jdump_state* jd, const char* name, struct nlattr* attr)
{
	if (attr) {
		jdump_put_key(jd, name);
		jdump_put_pod(jd, NULL, -1, false);
		unsigned char* addr = nla_data(attr);
		fprintf(jd->fp, "\"%02x:%02x:%02x:%02x:%02x:%02x\"",
			addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
	}
}
