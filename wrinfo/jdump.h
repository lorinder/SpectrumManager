#ifndef JDUMP_H
#define JDUMP_H

/**	@file	jdump.h
 *
 *	Minimalist JSON dumping tool.  Generates JSON output from a
 *	series of simple statements.  This is minimal and does not take
 *	care of proper quoting of strings and such things.
 */

#include <stdio.h>
#include <stdbool.h>

typedef enum {
	JDUMP_TP_POD,		// String, number, bool or NULL
	JDUMP_TP_ARRAY,
	JDUMP_TP_OBJECT
} jdump_type;

typedef struct {
	/** The JSON type on stack.
	 *
	 *  Only array and object types are aggregate, so those are the
	 *  only ones who can appear on the stack.
	 */
	jdump_type tp;

	/** Dump state of the type on stack.
	 *
	 *  For array: 0 if no entry was added to the array yet, 1 if at
	 *  least one entry was added.
	 *
	 *  For object: 0 if no entry was added to the object yet, 1 if
	 *  the last thing that was added was a key, 2 if the last thing
	 *  that was added was a value to a key.
	 */
	int state;
} jdump_stack_entry;

#define JDUMP_MAX_STACK_DEPTH		16

typedef struct jdump_state_S {
	FILE* fp;
	int stack_depth;
	jdump_stack_entry stack[JDUMP_MAX_STACK_DEPTH];
} jdump_state;

jdump_state jdump_create(FILE* fp);
int jdump_put_pod(jdump_state* ds, const char* rep, ssize_t len, bool quot_marks);
int jdump_put_array(jdump_state* ds);
int jdump_close_array(jdump_state* ds);
int jdump_put_object(jdump_state* ds);
int jdump_put_key(jdump_state* ds, const char* key);
int jdump_close_object(jdump_state* ds);
int jdump_done(jdump_state* ds);

#endif /* JSON_DUMP_H */
