#ifndef WIFI_INTERFACE_H
#define WIFI_INTERFACE_H

struct nlcctx;
struct jdump_state_S;
typedef struct jdump_state_S jdump_state;

int wifi_get_interface_info(struct nlcctx* ctx, jdump_state* jd);

#endif /* WIFI_INTERFACE_H */
