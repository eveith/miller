#include <stdio.h>
#include <string.h>
#include "lib/mlrutil.h"
#include "lib/string_builder.h"
#include "containers/slls.h"
#include "input/peek_file_reader.h"

#define TERMIND_RS  0x1111
#define TERMIND_FS  0x2222
#define TERMIND_EOF 0x3333

typedef struct _field_wrapper_t {
	char* contents;
	int   termination_indicator;
} field_wrapper_t;

typedef struct _record_wrapper_t {
	slls_t* contents;
	int   at_eof;
} record_wrapper_t;

static field_wrapper_t get_csv_field_not_dquoted(peek_file_reader_t* pfr, string_builder_t* psb) {
	field_wrapper_t wrapper;
	// xxx need pfr_advance_past_or_die ...
	// xxx "\"," etc. will be encoded in the rfc_csv_reader_t ctor -- this is just sketch
	while (TRUE) {
		if (pfr_at_eof(pfr)) {
			wrapper.contents = sb_finish(psb);
			wrapper.termination_indicator = TERMIND_EOF;
			printf("---- NDQ EOF FLD >>%s<<\n", wrapper.contents);
			return wrapper;
		} else if (pfr_next_is(pfr, ",", 1)) {
			if (!pfr_advance_past(pfr, ",")) {
				fprintf(stderr, "xxx k0d3 me up b04k3n b04k3n b04ken %d\n", __LINE__);
				exit(1);
			}
			wrapper.contents = sb_finish(psb);
			wrapper.termination_indicator = TERMIND_FS;
			printf("---- NDQ FS FLD >>%s<<\n", wrapper.contents);
			return wrapper;
		} else if (pfr_next_is(pfr, "\n", 1)) {
			if (!pfr_advance_past(pfr, "\n")) {
				fprintf(stderr, "xxx k0d3 me up b04k3n b04k3n b04ken %d\n", __LINE__);
				exit(1);
			}
			wrapper.contents = sb_finish(psb);
			wrapper.termination_indicator = TERMIND_RS;
			printf("---- NDQ RS FLD >>%s<<\n", wrapper.contents);
			return wrapper;
		} else {
			sb_append_char(psb, pfr_read_char(pfr));
		}
	}
}

static field_wrapper_t get_csv_field_dquoted(peek_file_reader_t* pfr, string_builder_t* psb) {
	field_wrapper_t wrapper;
	// xxx need pfr_advance_past_or_die ...
	if (!pfr_advance_past(pfr, "\"")) {
		fprintf(stderr, "xxx k0d3 me up b04k3n b04k3n b04ken %d\n", __LINE__);
		exit(1);
	}
	while (TRUE) {
		if (pfr_at_eof(pfr)) {
			// xxx imbalanced-dquote error
			fprintf(stderr, "xxx k0d3 me up b04k3n b04k3n b04ken %d\n", __LINE__);
			exit(1);
		} else if (pfr_next_is(pfr, "\"\xff", 2)) {
			if (!pfr_advance_past(pfr, "\"\xff")) {
				fprintf(stderr, "xxx k0d3 me up b04k3n b04k3n b04ken %d\n", __LINE__);
				exit(1);
			}
			wrapper.contents = sb_finish(psb);
			wrapper.termination_indicator = TERMIND_EOF;
			return wrapper;
		} else if (pfr_next_is(pfr, "\",", 2)) {
			if (!pfr_advance_past(pfr, "\",")) {
				fprintf(stderr, "xxx k0d3 me up b04k3n b04k3n b04ken %d\n", __LINE__);
				exit(1);
			}
			wrapper.contents = sb_finish(psb);
			wrapper.termination_indicator = TERMIND_FS;
			return wrapper;
		} else if (pfr_next_is(pfr, "\"\n", 2)) {
			if (!pfr_advance_past(pfr, "\"\n")) {
				fprintf(stderr, "xxx k0d3 me up b04k3n b04k3n b04ken %d\n", __LINE__);
				exit(1);
			}
			wrapper.contents = sb_finish(psb);
			wrapper.termination_indicator = TERMIND_RS;
			return wrapper;
		} else {
			sb_append_char(psb, pfr_read_char(pfr));
		}
	}
}

// needs to return the contents, as well as what kind of termination followed
// (and was consumed): FS, RS, EOF.
field_wrapper_t get_csv_field(peek_file_reader_t* pfr, string_builder_t* psb) {
	field_wrapper_t wrapper;
	if (pfr_at_eof(pfr)) {
		wrapper.contents = NULL;
		wrapper.termination_indicator = TERMIND_EOF;
		return wrapper;
	} else if (pfr_next_is(pfr, "\"", 1)) {
		return get_csv_field_dquoted(pfr, psb);
	} else {
		return get_csv_field_not_dquoted(pfr, psb);
	}
}

record_wrapper_t get_csv_record(peek_file_reader_t* pfr, string_builder_t* psb) {
	slls_t* fields = slls_alloc();
	record_wrapper_t rwrapper;
	rwrapper.contents = fields;
	rwrapper.at_eof = FALSE;
	while (TRUE) {
		field_wrapper_t fwrapper = get_csv_field(pfr, psb);
		if (fwrapper.contents != NULL)
			printf("FLD >>%s<<\n", fwrapper.contents);
		if (fwrapper.termination_indicator == TERMIND_EOF) {
			rwrapper.at_eof = TRUE;
			break;
		}
		slls_add_with_free(fields, fwrapper.contents);
		if (fwrapper.termination_indicator != TERMIND_FS)
			break;
	}
	printf("EOR\n");
	return rwrapper;
}

int main() {
	FILE* fp = stdin;
	peek_file_reader_t* pfr = pfr_alloc(fp, 32);
	string_builder_t sb;
	string_builder_t* psb = &sb;
	sb_init(psb, 1024);

	while (TRUE) {
		record_wrapper_t rwrapper = get_csv_record(pfr, psb);
		printf("++++ [NF=%d]", rwrapper.contents->length);
		slls_print(rwrapper.contents);
		slls_free(rwrapper.contents);
		printf("\n");
		if (rwrapper.at_eof)
			break;
	}

	return 0;
}