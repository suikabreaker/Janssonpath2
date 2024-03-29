#include <janssonpath_error.h>
#include "private/common.h"
jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_ok;
jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_unknown;
JANSSONPATH_NO_EXPORT jsonpath_error_t jsonpath_error_nullptr;
jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_to_array_non_collection;
jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_from_array_collection;

JANSSONPATH_NO_EXPORT jsonpath_error_t
jsonpath_error_eof_escape(const char* position);
JANSSONPATH_NO_EXPORT jsonpath_error_t
jsonpath_error_zero_length_escape(const char* position);
JANSSONPATH_NO_EXPORT jsonpath_error_t
jsonpath_error_regex_compile_error(unsigned errorcode, const char* error_msg);
JANSSONPATH_NO_EXPORT jsonpath_error_t
jsonpath_error_function_not_found(const char* function_name);
jsonpath_error_t JANSSONPATH_NO_EXPORT
json_error_unmatched_bracked(const char* position);
jsonpath_error_t JANSSONPATH_NO_EXPORT
json_error_expecting_index(const char* position);