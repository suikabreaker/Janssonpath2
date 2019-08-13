#ifndef JANSSONPATH_DEPRECATED_H
#define JANSSONPATH_DEPRECATED_H
#include "janssonpath_conf.h"
#include "janssonpath_export.h"
#include "jansson.h"
// For backward compatibility.
// Especially for shared library.
#ifdef __cplusplus
extern "C" {
#endif

typedef struct JANSSONPATH_DEPRECATED path_result {
	json_t* result;
	int is_collection;
} path_result;


JANSSONPATH_DEPRECATED_EXPORT path_result json_path_get_distinct_cond(json_t* json, const char* path, bool classical);
JANSSONPATH_DEPRECATED_EXPORT path_result json_path_get_distinct(json_t* json, const char* path);
JANSSONPATH_DEPRECATED_EXPORT path_result json_path_get_distinct_modern(json_t* json, const char* path);

JANSSONPATH_DEPRECATED_EXPORT json_t* json_path_get_cond(json_t* json, const char* path, bool classical);
JANSSONPATH_DEPRECATED_EXPORT json_t* json_path_get(json_t* json, const char* path);
JANSSONPATH_DEPRECATED_EXPORT json_t* json_path_get_modern(json_t* json, const char* path);

#ifdef __cplusplus
}
#endif
#endif
