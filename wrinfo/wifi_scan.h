#ifndef WIFI_SCAN_H
#define WIFI_SCAN_H

/**
 *	Routines to perform a wifi scan
 */

struct nlcctx;
struct jdump_state_S;
typedef struct jdump_state_S jdump_state;

int wifi_scan(struct nlcctx* ctx);
int wifi_get_scan_results(struct nlcctx* ctx, jdump_state* jd);

#endif /* WIFI_SCAN_H */
