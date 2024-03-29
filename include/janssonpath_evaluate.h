#ifndef JANSSONPATH_EVALUATE
#define JANSSONPATH_EVALUATE

#include <stdbool.h>
#include "jansson.h"
#include "janssonpath_conf.h"
#include "janssonpath_export.h"
#include "janssonpath_error.h"

#ifdef __cplusplus
extern "C" {
#endif

// By some means it's compatible with jansson1, as jansson1's jsonpath does not generate right value and constant.
// But it's not reliable especially for input with jansson2 extensions. Thus we need translation for old API.

typedef struct jsonpath_result_t {
	json_t* value;
	// Whether it is an real array or a collection of json nodes.
	// Operation to collection would be applied to all of its elements, and if results are collections, they are merged into one.
	bool is_collection : 1;
	// A path generates left value which makes sense to modify; and operation like + - * / generates right value.
	// Note that collection being left value means every element of it is a left value.
	// It's for reference only, as some opration like to_list would lose information about left/right value of its content.
	bool is_right_value : 1;
	// Literals are constant; result of operation to constants are constant. It could be used for optimization.
	// It could be used for optimization, and it tells you if it's safe to cache its value.
	bool is_constant : 1;
}jsonpath_result_t;

#define jsonpath_incref(in) (json_incref((in).value),(in))

// instead of jsonpath_result_release we decref it.
#define jsonpath_decref(in) json_decref(in.value)

// As a loose constrain, jsonpath_functions should not json_decref or modify its input. (json_incref is allowed)
// jsonpath_function should return a new reference.
typedef json_t* (*jsonpath_callable_plain_t)(json_t**, size_t);

// names[i] <-> functions[i] is a one to one map.
typedef struct jsonpath_function_table_t{
	const char* *names;
	const jsonpath_callable_plain_t* functions;
	size_t size;
}jsonpath_function_table_t;

typedef json_t* (*jsonpath_callable_func_t)(json_t**, size_t, void*);
typedef struct jsonpath_callable_bind_t{
	jsonpath_callable_func_t function;
	void* bind;
}jsonpath_callable_bind_t;


typedef enum jsonpath_callable_tag_t{
	JSONPATH_CALLABLE_PLAIN, JSONPATH_CALLABLE_BIND, JSONPATH_CALLABLE_MAX
} jsonpath_callable_tag_t;

typedef struct jsonpath_function_lookup_t {
	jsonpath_callable_tag_t tag;
	union{
		jsonpath_function_table_t plain_table;
		struct {
			jsonpath_callable_bind_t(*bind_map)(const char*, void*);
			void* bind_context;
		};
	};
}jsonpath_function_lookup_t;

typedef json_t* (*jsonpath_variable_lookup_t)(const char*);

typedef struct jsonpath_symbol_lookup_t {
	jsonpath_function_lookup_t function_lookup;
	jsonpath_variable_lookup_t variable_lookup;
}jsonpath_symbol_lookup_t;

struct jsonpath_t;
typedef struct jsonpath_t jsonpath_t;
JANSSONPATH_EXPORT jsonpath_result_t jsonpath_evaluate(json_t* root, jsonpath_t* jsonpath, jsonpath_symbol_lookup_t* symbols, jsonpath_error_t* error);

#ifdef __cplusplus
}
#endif
#endif
