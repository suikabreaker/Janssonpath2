#include "private/jansson_memory.h"

static void* json_default_malloc(size_t len);
static void json_default_free(void* mem);
json_malloc_t JANSSONPATH_NO_EXPORT do_malloc = json_default_malloc;
json_free_t JANSSONPATH_NO_EXPORT do_free = json_default_free;

static void* json_default_malloc(size_t len) {
	json_get_alloc_funcs(&do_malloc, &do_free);
	if (!do_malloc) do_malloc = malloc;
	if (!do_free) do_free = free;
	return do_malloc(len);
}

static void json_default_free(void* mem) {
	json_get_alloc_funcs(&do_malloc, &do_free);
	if (!do_malloc) do_malloc = malloc;
	if (!do_free) do_free = free;
	do_free(mem);
}

void JANSSONPATH_EXPORT json_path_set_alloc_funcs(json_malloc_t malloc_fn, json_free_t free_fn) {
	do_malloc = malloc_fn;
	do_free = free_fn;
}
void JANSSONPATH_EXPORT json_path_get_alloc_funcs(json_malloc_t* malloc_fn, json_free_t* free_fn) {
	*malloc_fn = do_malloc;
	*free_fn = do_free;
}
