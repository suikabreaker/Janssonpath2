#include "private/common.h"
#include "janssonpath_deprecated.h"
#include "janssonpath.h"

JANSSONPATH_DEPRECATED_EXPORT json_t* json_path_get(json_t* json, const char* path) {
	return NULL;
}
JANSSONPATH_DEPRECATED_EXPORT path_result json_path_get_distinct(json_t* json, const char* path){
	path_result ret = { NULL,false };
	return ret;
}

