#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <unistd.h>

#include "jdump.h"
#include "nlcctx.h"
#include "wifi_interface.h"
#include "wifi_scan.h"
#include "wifi_survey.h"

static void usage()
{
	puts(	"wrinfo -- Utility to get Wi-Fi radio information\n"
		"\n"
		"Collects information on Wi-Fi radio, such as scan data\n"
		"to discover other visible APs as well as a channel survey\n"
		"with channel occupancy and noise level information\n"
		"\n"
		"  -h       display this help and exit\n"
		"  -i <if>  wifi networ interface to query\n"
	);
}

static void dump_meta_info(jdump_state* jd, int argc, char** argv)
{
	jdump_put_object(jd);

	/* Record command line */
	jdump_put_key(jd, "cmdline");
	jdump_put_pod(jd, NULL, -1, false);
	putc('"', stdout);
	for (int i = 0; i < argc; ++i) {
		if (i > 0)
			putc(' ', stdout);
		printf("%s", argv[i]);
	}
	putc('"', stdout);

	/* Record time the sample was taken */
	{
		const time_t t = time(NULL);

		jdump_put_key(jd, "time_human");
		jdump_put_pod(jd, ctime(&t), -1, true);
		jdump_put_key(jd, "time_unix");
		jdump_put_pod(jd, NULL, -1, false);
		printf("%lld", (long long int)t);
	}

	jdump_close_object(jd);
}

int main(int argc, char** argv)
{
	/* Settings, default values */
	char* net_if = strdup("wlan0");
	bool do_scan = false;

	/* Parse command line arguments */
	int c;
	while ((c = getopt(argc, argv, "hi:s")) != -1) {
		switch (c) {
		case 'h':
			usage();
			return EXIT_SUCCESS;
		case 'i':
			free(net_if);
			net_if = strdup(optarg);
			if (net_if == NULL) {
				fprintf(stderr, "Error:  malloc failed.\n");
				return EXIT_FAILURE;
			}
			break;
		case 's':
			do_scan = true;
			break;
		case '?':
			return EXIT_FAILURE;
		};
	}

	/* Create the context */
	struct nlcctx* ctx = nlcctx_create(net_if);
	if (ctx == NULL) {
		/* Error already printed */
		return EXIT_FAILURE;
	}

	/* Create output */
	jdump_state jd = jdump_create(stdout);
	jdump_put_object(&jd);

	/* Get interface information */
	jdump_put_key(&jd, "interface");
	wifi_get_interface_info(ctx, &jd);

	/* Perform channel scan + Record results */
	if (do_scan)
		wifi_scan(ctx);
	jdump_put_key(&jd, "scan");
	wifi_get_scan_results(ctx, &jd);

	/* Get Survey */
	jdump_put_key(&jd, "survey");
	wifi_get_survey_results(ctx, &jd);

	/* Get meta information */
	jdump_put_key(&jd, "meta");
	dump_meta_info(&jd, argc, argv);

	jdump_close_object(&jd); // end of global object
	jdump_done(&jd);

	nlcctx_free(ctx);
	return EXIT_SUCCESS;
}
