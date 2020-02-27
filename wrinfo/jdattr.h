#ifndef JDATTR_H
#define JDATTR_H

/**	@file	jdattr.h
 *
 *	Utilities to print netlink attributes as jdump (key, value)
 *	pairs.  These utilities are used to print out results found by
 *	the various NL80211 querying callbacks.
 */

#include "jdump.h"

struct nlattr;

void jdattr_flag(jdump_state* jd, const char* name, struct nlattr* attr);

void jdattr_i8(jdump_state* jd, const char* name, struct nlattr* attr);
void jdattr_u32(jdump_state* jd, const char* name, struct nlattr* attr);
void jdattr_u64(jdump_state* jd, const char* name, struct nlattr* attr);

void jdattr_str(jdump_state* jd, const char* name, struct nlattr* attr);
void jdattr_macaddr(jdump_state* jd, const char* name, struct nlattr* attr);

#endif /* JDATTR_H */
