#ifndef LEXEME_H
#define LEXEME_H
#include "common.h"

string_slice LOCAL next_lexeme(
	const char** ps_begin, const char* s_end, jsonpath_error_t* error
);
string_slice LOCAL next_nonspace_lexeme(
	const char** ps_begin, const char* s_end, jsonpath_error_t* error
);

#endif