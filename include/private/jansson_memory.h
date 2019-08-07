#ifndef JANSSON_MEMORY_H
#define JANSSON_MEMORY_H
#include "common.h"
#include "jansson.h"
extern json_malloc_t JANSSONPATH_NO_EXPORT do_malloc;
extern json_free_t JANSSONPATH_NO_EXPORT do_free;

void JANSSONPATH_EXPORT jsonpath_set_alloc_funcs(json_malloc_t malloc_fn,
                                                 json_free_t free_fn);
void JANSSONPATH_EXPORT jsonpath_get_alloc_funcs(json_malloc_t* malloc_fn,
                                                 json_free_t* free_fn);

#endif