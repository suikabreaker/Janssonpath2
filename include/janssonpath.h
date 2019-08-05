#ifndef JANSSONPATH_H
#define JANSSONPATH_H
#include "jansson.h"
#include "janssonpath_conf.h"
#include "janssonpath_error.h"
#include "janssonpath_export.h"
#include "janssonpath_evaluate.h"
// for you can recompile without modify original code
#include "janssonpath_deprecated.h"
#ifdef __cplusplus
extern "C"{
#endif

struct jsonpath_t;
typedef struct jsonpath_t jsonpath_t;

// Set and get alloc_functions of janssonpath. Default to jansson's setting, and malloc/free if that is not applicable.
void JANSSONPATH_EXPORT jsonpath_set_alloc_funcs(json_malloc_t malloc_fn, json_free_t free_fn);
void JANSSONPATH_EXPORT jsonpath_get_alloc_funcs(json_malloc_t* malloc_fn, json_free_t* free_fn);

// Set to true to tolerate encode error. Default to false.
void JANSSONPATH_EXPORT jsonpath_set_encode_recoverable(bool value);

// Jsonpath is not supposed to be very long, so we do not provide interface other than string input.

// Compile given jsonpath. *pjsonpath_end set to NULL for Null-terminated string, or one pass the end of string.
// *pjsonpath_end will be set to the end of parsed jsonpath for you to check or further process.
// pjsonpath_end set to NULL for Null-terminated string and treat additional charactor after jsonpath as error.
// Note that even *pjsonpath_end is set to non-NULL, \0 will still be treat as string end
JANSSONPATH_EXPORT jsonpath_t* jsonpath_compile_ranged(const char* jsonpath_begin, const char** pjsonpath_end, jsonpath_error_t* error);
// Compile given jsonpath.
JANSSONPATH_EXPORT jsonpath_t* jsonpath_compile(const char* jsonpath_begin, jsonpath_error_t* error);
// Parse jsonpath expression from suffix of *p_jsonpath_begin. *p_jsonpath_begin will be returned end of jsonpath expression.
// You can use this to parse jsonpath as a part of your parser.
// Note if a non-recognized token occurs it's treated as error(even if it's at the end of jsonpath)
// To avoid this you can always surround jsonpath with brackets and start jsonpath_parse inside the surrounding brackets.
JANSSONPATH_EXPORT jsonpath_t* jsonpath_parse(const char** p_jsonpath_begin, jsonpath_error_t* error);

// Release the jsonpath. Do nothing to NULL.
void JANSSONPATH_EXPORT jsonpath_release(jsonpath_t* jsonpath);

#ifdef __cplusplus
}
#endif
#endif
