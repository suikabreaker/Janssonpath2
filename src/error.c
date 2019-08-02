#include "private/common.h"
#include "janssonpath_error.h"

jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_ok = {
	0, "OK", NULL
};

jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_unknown = {
	1, "unknown", NULL
};
