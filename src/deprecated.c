#include "private/common.h"
#include "janssonpath_deprecated.h"
#include "janssonpath.h"

JANSSONPATH_DEPRECATED_EXPORT path_result json_path_get_distinct(json_t* json, const char* path){
	path_result ret = { NULL,false };

	jsonpath_error_t error;
	jsonpath_t* jsonpath = jsonpath_compile_ranged(path, NULL, &error);
	if (error.abort) return ret;

	jsonpath_result_t result = jsonpath_evaluate(json, jsonpath, NULL, &error);
	if (error.abort) {
		goto jsonpath_;
	}

	ret.result = result.value;
	ret.is_collection = result.is_collection;
jsonpath_:
	jsonpath_release(jsonpath);
	return ret;
}

JANSSONPATH_DEPRECATED_EXPORT json_t* json_path_get(json_t* json, const char* path) {
	json_t* ret = NULL;

	jsonpath_error_t error;
	jsonpath_t* jsonpath = jsonpath_compile_ranged(path, NULL, &error);
	if (error.abort) return ret;

	jsonpath_result_t result = jsonpath_evaluate(json, jsonpath, NULL, &error);
	if (error.abort) {
		goto jsonpath_;
	}

	ret = result.value;
jsonpath_:
	jsonpath_release(jsonpath);
	return ret;
}
