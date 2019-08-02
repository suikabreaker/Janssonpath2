#ifndef JANSSONPATH_H
#define JANSSONPATH_H
#include "jansson.h"
#include "janssonpath_conf.h"
#include "janssonpath_error.h"
#include "janssonpath_export.h"

struct jsonpath_t;
typedef struct jsonpath_t jsonpath_t;

void JANSSONPATH_EXPORT json_path_set_alloc_funcs(json_malloc_t malloc_fn, json_free_t free_fn);
void JANSSONPATH_EXPORT json_path_get_alloc_funcs(json_malloc_t* malloc_fn, json_free_t* free_fn);

// Jsonpath is not supposed to be very long, so we do not provide interface other than string input.

// Compile given jsonpath. *pjsonpath_end set to NULL for Null-terminated string, or one pass the end of string.
// *pjsonpath_end will be set to the end of parsed jsonpath for you to check or further process.
// pjsonpath_end set to NULL for Null-terminated string and treat additional charactor after jsonpath as error.
// note that even *pjsonpath_end is set to non-NULL, \0 will still be treat as string end
JANSSONPATH_EXPORT jsonpath_t* jsonpath_compile_ranged(const char* jsonpath_begin, const char** pjsonpath_end, jsonpath_error_t* error);
void JANSSONPATH_EXPORT jsonpath_release(jsonpath_t* jsonpath);

// set to true to tolerate encode error.
void JANSSONPATH_EXPORT jsonpath_set_encode_recoverable(bool value);

#endif