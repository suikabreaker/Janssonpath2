#ifndef JANSSONPATH_PRIVATE_H
#define JANSSONPATH_PRIVATE_H
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "janssonpath_conf.h"
#include "janssonpath_error.h"
#include "janssonpath_export.h"

// end==NULL means till the end(NULL terminated)
// end is one pass the last
typedef struct string_slice {
    const char* begin;
    const char* end;
} string_slice;

#define IS_SLICE_EMPTY(slice) (!(slice).begin)
#define FMT_SLICE ".*s"
#define SLICE_SIZE(slice)                                                 \
    ((slice).begin ? ((slice).end ? (size_t)((slice).end - (slice).begin) \
                                  : strlen(slice.begin))                  \
                   : 0u)
#define SLCIE_OUT(slice) (int)SLICE_SIZE(slice), (slice).begin

extern jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_ok;
extern jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_unknown;

#endif
