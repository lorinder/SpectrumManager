#include <string.h>

#include "jdump.h"

/* Stack managment helpers */

static jdump_stack_entry* pop(jdump_state* ds)
{
	if (ds->stack_depth <= 0) {
		fprintf(stderr, "Error (jdump):  Stack underflow.\n");
		return NULL;
	}

	--ds->stack_depth;
	return ds->stack + ds->stack_depth;
}

static jdump_stack_entry* push(jdump_state* ds)
{
	/* Push array entry on stack */
	if (ds->stack_depth >= JDUMP_MAX_STACK_DEPTH) {
		fprintf(stderr, "Error (jdump):  Max stack depth exceeded.\n");
		return NULL;
	}
	jdump_stack_entry* se = ds->stack + ds->stack_depth;
	++ds->stack_depth;
	return se;
}

/* Pretty-print helpers */

static void indent(jdump_state* ds)
{
	putc('\n', ds->fp);
	for (int i = 0; i < ds->stack_depth; ++i) {
		fprintf(ds->fp, "  ");
	}
}

/* API implementation */

jdump_state jdump_create(FILE* fp)
{
	jdump_state ret = {
		.fp = fp,
		.stack_depth = 0,
	};
	return ret;
}

static void write_str(FILE* fp, const char* str, ssize_t len, bool quot_marks)
{
	if (quot_marks)
		putc('"', fp);

	/* Place data, quoting if necessary */
	for (int i = 0; i < len; ++i) {
		int c = (unsigned char)str[i];
		if (c <= 0x1f || c == '\\' || c == '"') {
			switch (c) {
			case '\n':
				fprintf(fp, "\\n");
				break;
			case '\\':
			case '"':
				fprintf(fp, "\\%c", c);
				break;
			default:
				fprintf(fp, "\\u%04X", c);
			}
		} else {
			putc(c, fp);
		}
	}

	if (quot_marks)
		putc('"', fp);
}

int jdump_put_pod(jdump_state* ds, const char* rep, ssize_t len, bool quot_marks)
{
	if (ds->stack_depth != 0) {
		/* Validate and add separators (commas) for containing
		 * aggregate types.
		 */
		const int sd = ds->stack_depth;
		jdump_stack_entry* se = &ds->stack[sd - 1];
		switch (se->tp) {
		case JDUMP_TP_ARRAY:
			if (se->state > 0) {
				putc(',', ds->fp);
			}
			indent(ds);
			se->state = 1;
			break;
		case JDUMP_TP_OBJECT:
			switch (se->state) {
			case 0:
			case 2:
				fprintf(stderr, "Error (jdump):  Need to put a key "
				  "before putting a POD into an object.\n");
				return -1;
			case 1:
				se->state = 2;
				/* All good, ready to put the POD */
				break;
			}
			break;
		default:
			fprintf(stderr, "Internal Error:%s:%d:  "
			  "Unexpected Type on stack.\n", __FILE__, __LINE__);
			return -1;
		}
	}

	/* Now place the POD value */
	if (rep) {
		if (len == -1)
			len = strlen(rep);
		write_str(ds->fp, rep, len, quot_marks);
	}

	return 0;
}

int jdump_put_array(jdump_state* ds)
{
	/* Print opening brace and update state of current top stack
	 * entry.
	 *
	 * We're abusing jdump_put_pod for these tasks.
	 */
	jdump_put_pod(ds, "[ ", -1, false);

	/* Push array on stack */
	jdump_stack_entry* se = push(ds);
	if (se == NULL)
		return -1;
	se->tp = JDUMP_TP_ARRAY;
	se->state = 0;

	return 0;
}

int jdump_close_array(jdump_state* ds)
{
	jdump_stack_entry* se = pop(ds);
	if (ds == NULL)
		return -1;

	/* Validate */
	if (se->tp != JDUMP_TP_ARRAY) {
		fprintf(stderr, "Error (jdump):  jdump_close_array() called "
		  "on non-array.\n");
		return -1;
	}

	/* Print closing bracket */
	if (se->state != 0)
		indent(ds);
	putc(']', ds->fp);

	return 0;
}

int jdump_put_object(jdump_state* ds)
{
	/* Print opening brace; update top stack entry;
	 * as in jdump_put_array, we abuse jdump_pod_for this.
	 */
	jdump_put_pod(ds, "{ ", -1, false);

	/* Push object on stack */
	jdump_stack_entry* se = push(ds);
	if (se == NULL)
		return -1;
	se->tp = JDUMP_TP_OBJECT;
	se->state = 0;

	return 0;
}

int jdump_put_key(jdump_state* ds, const char* key)
{
	if (ds->stack_depth == 0
	    || ds->stack[ds->stack_depth - 1].tp != JDUMP_TP_OBJECT)
	{
		fprintf(stderr, "Error (jdump):  jdump_put_key() called "
		  "outside of object.\n");
		return -1;
	}

	/* Validate */
	jdump_stack_entry* se = ds->stack + ds->stack_depth - 1;
	if (se->state == 1) {
		fprintf(stderr, "Error (jdump):  Two jdump_put_key() calls "
		  "without intermediate placement of value.\n");
		return  -1;
	}

	/* Print */
	if (se->state == 2) {
		putc(',', ds->fp);
	}
	indent(ds);
	se->state = 1;
	fprintf(ds->fp, "\"%s\": ", key);

	return 0;
}

int jdump_close_object(jdump_state* ds)
{
	jdump_stack_entry* se = pop(ds);
	if (ds == NULL)
		return -1;

	/* Validate */
	if (se->tp != JDUMP_TP_OBJECT) {
		fprintf(stderr, "Error (jdump):  jdump_close_object() called "
		  "on non-object.\n");
		return -1;
	}
	if (se->state != 0 && se->state != 2) {
		fprintf(stderr, "Error (jdump):  jdump_close_object() called "
		  "without placing last value.\n");
		return -1;
	}

	/* Print closing brace */
	if (se->state != 0)
		indent(ds);
	putc('}', ds->fp);

	return 0;
}

int jdump_done(jdump_state* ds)
{
	if (ds->stack_depth != 0) {
		fprintf(stderr, "Error (jdump): jdump_done() called with "
		  "non-empty stack.\n");
		return -1;
	}
	putc('\n', stdout);
	return 0;
}
