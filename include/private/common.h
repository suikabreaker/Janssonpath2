#ifndef JANSSONPATH_PRIVATE_H
#define JANSSONPATH_PRIVATE_H
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#include "janssonpath_conf.h"
#include "janssonpath_export.h"
#include "janssonpath_error.h"

// end==NULL means till the end(NULL terminated)
// end is one pass the last
typedef struct string_slice {
	const char* begin;
	const char* end;
}string_slice;

#define IS_SLICE_EMPTY(slice) (!(slice).begin)
#define FMT_SLICE ".*s"
#define SLICE_SIZE(slice) ((slice).end?((slice).end-(slice).begin):strlen(slice.begin))
#define SLCIE_OUT(slice) (int)SLICE_SIZE(slice), (slice).begin

static int slice_cstr_cmp(string_slice slice, const char* c_str) {
	size_t len = SLICE_SIZE(slice);
	int cmp = strncmp(slice.begin, c_str, len);
	if (cmp) return cmp;
	// cmp == 0
	return -(int)(unsigned char)c_str[len];
}


extern jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_ok;
extern jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_unknown;

#endif
