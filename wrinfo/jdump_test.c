#include <stdio.h>
#include <stdlib.h>

#include "jdump.h"

#define CHECKED(x) \
	do { \
		if ((x) < 0) { \
			fprintf(stderr, "Failure was on line %d of %s\n", \
			  __LINE__, __FILE__); \
			exit(1); \
		} \
	} while(0)

static void simple_array(jdump_state* j)
{
	CHECKED(jdump_put_array(j));
	CHECKED(jdump_put_pod(j, "1", -1, false));
	CHECKED(jdump_put_pod(j, "2", -1, false));
	CHECKED(jdump_put_pod(j, "3", -1, false));
	CHECKED(jdump_close_array(j));
}

static void simple_object(jdump_state* j)
{
	CHECKED(jdump_put_object(j));
	CHECKED(jdump_put_key(j, "a"));
	CHECKED(jdump_put_pod(j, "12", -1, false));
	CHECKED(jdump_put_key(j, "b"));
	CHECKED(jdump_put_pod(j, "13", -1, false));
	CHECKED(jdump_put_key(j, "c"));
	CHECKED(jdump_put_pod(j, "chicken", -1, true));
	CHECKED(jdump_close_object(j));
}

int main(int argc, char** argv)
{
	puts("Testing tool for the jdump module.");
	jdump_state Jd = jdump_create(stdout);
	jdump_state* j = &Jd;

	puts("\nJdump output for a simple array [ 1, 2, 3 ]:");
	simple_array(j);
	CHECKED(jdump_done(j));

	puts("\nJdump output for a simple object { \"a\": 12, \"b\": 13, \"c\": 14 }:");
	simple_object(j);
	CHECKED(jdump_done(j));

	puts("\nJdump output for an empty array []:");
	CHECKED(jdump_put_array(j));
	CHECKED(jdump_close_array(j));
	CHECKED(jdump_done(j));

	puts("\nJdump output for an empty object {}:");
	CHECKED(jdump_put_object(j));
	CHECKED(jdump_close_object(j));
	CHECKED(jdump_done(j));

	puts("\nJdump output for an array of arrays and objects:");
	CHECKED(jdump_put_array(j));
	simple_array(j);
	jdump_put_array(j);
	jdump_close_array(j);
	simple_object(j);
	CHECKED(jdump_close_array(j));
	CHECKED(jdump_done(j));

	puts("\nJdump output for object of arrays and objects:");
	CHECKED(jdump_put_object(j));
	jdump_put_key(j, "arr");
	simple_array(j);
	jdump_put_key(j, "obj");
	simple_object(j);
	CHECKED(jdump_close_object(j));
	CHECKED(jdump_done(j));

	return 0;
}
