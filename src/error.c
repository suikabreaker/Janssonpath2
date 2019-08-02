#include "private/common.h"
#include "janssonpath_error.h"

jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_ok = {
	false, 0, "OK", NULL
};

jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_unknown = {
	true, 1, "unknown", NULL
};

jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_eof_escape(const char* position){
	jsonpath_error_t ret = { true,0x22,"unexpected eof while in escape sequence",(void*)position };
	return ret;
};

jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_zero_length_escape(const char* position) {
	jsonpath_error_t ret = { true, 0x23, "escape sequnce with length of zero", (void*)position };
	return ret;
}

