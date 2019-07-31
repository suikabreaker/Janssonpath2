#ifndef JANSSONPATH_ERROR
#define JANSSONPATH_ERROR

typedef struct jsonpath_error_t {
	int code;
	const char* reason;
	void* extra;
}jsonpath_error_t;

extern jsonpath_error_t LOCAL jsonpath_error_ok;
extern jsonpath_error_t LOCAL jsonpath_error_unknown;

#endif // !JANSSONPATH_ERROR
