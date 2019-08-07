#ifndef JANSSONPATH_ERROR
#define JANSSONPATH_ERROR

#include <stdbool.h>

typedef struct jsonpath_error_t {
	bool abort; // true if not recoverable
	long long unsigned int code; // error code. 0 for OK
	const char* reason; // Null-terminated string for description text of error
	const void* extra; // extra information. typically position of error occuring
}jsonpath_error_t;

#endif // !JANSSONPATH_ERROR
