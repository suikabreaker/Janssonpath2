#ifndef JANSSONPATH_PRIVATE_H
#define JANSSONPATH_PRIVATE_H
#include <assert.h>
#include <stdbool.h>

#include "janssonpath_conf.h"

#ifdef GCC
#define EXPORT __attribute__((visibility ("default")))
#define IMPORT __attribute__((visibility ("default")))
#define LOCAL __attribute__((visibility ("hidden")))
#elif defined MSVC
#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)
#define LOCAL 
#else
#define EXPORT
#define IMPORT
#define LOCAL
#endif

// end==NULL means till the end(NULL terminated)
// end is one pass the last
typedef struct string_slice {
	const char* begin;
	const char* end;
}string_slice;

#define IS_SLICE_EMPTY(slice) (!(slice).begin)
#define FMT_SLICE ".*s"
#define SLCIE_OUT(slice) (int)((slice).end-(slice).begin), (slice).begin

#include "janssonpath_error.h"

#endif
