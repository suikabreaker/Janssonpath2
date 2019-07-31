#ifndef JANSSONPATH_H
#define JANSSONPATH_H
#include "jansson.h"
#include "janssonpath_conf.h"
#include "janssonpath_error.h"

struct jsonpath_t;
typedef struct jsonpath_t jsonpath_t;

void json_path_set_alloc_funcs(json_malloc_t malloc_fn, json_free_t free_fn);
void json_path_get_alloc_funcs(json_malloc_t* malloc_fn, json_free_t* free_fn);

jsonpath_t* jsonpath_compile(const char* jsonpath_begin, const char* jsonpath_end, jsonpath_error_t* error);
void jsonpath_release(jsonpath_t* jsonpath);

#endif