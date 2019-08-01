#ifndef JANSSONPATH_PRIVATE_H
#define JANSSONPATH_PRIVATE_H
#include <assert.h>
#include <stdbool.h>

#include "janssonpath_conf.h"
#include "janssonpath_export.h"

// end==NULL means till the end(NULL terminated)
// end is one pass the last
typedef struct string_slice {
	const char* begin;
	const char* end;
}string_slice;

#define IS_SLICE_EMPTY(slice) (!(slice).begin)
#define FMT_SLICE ".*s"
#define SLCIE_OUT(slice) (int)((slice).end-(slice).begin), (slice).begin

#include "janssonpath_error.h"

#endif
