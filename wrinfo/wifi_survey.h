#ifndef WIFI_SURVEY_H
#define WIFI_SURVEY_H

struct nlcctx;
struct jdump_state_S;
typedef struct jdump_state_S jdump_state;

int wifi_get_survey_results(struct nlcctx* ctx, jdump_state* jd);

#endif /* WIFI_SURVEY_H */
