#ifndef LEXEME_H
#define LEXEME_H
#include "common.h"
#include <wctype.h>
#include <ctype.h>

#ifndef HAVE_ISWASCII
#define iswascii isascii
#endif

string_slice JANSSONPATH_NO_EXPORT next_lexeme(
	const char** ps_begin, const char* s_end, jsonpath_error_t* error
);
string_slice JANSSONPATH_NO_EXPORT next_nonspace_lexeme(
	const char** ps_begin, const char* s_end, jsonpath_error_t* error
);

// we assume it's ASCII compatiable encoded (E.g. UTF-8)
#define is_eof IS_SLICE_EMPTY

// expect them to be inlined
static bool is_number(string_slice input) {
	return !IS_SLICE_EMPTY(input) &&
		((isascii(input.begin[0]) && isdigit(input.begin[0])) ||
		((input.begin[0] == '.') && input.end - input.begin > 1));
}

static bool is_identifier(string_slice input) {
	if (IS_SLICE_EMPTY(input))return false;
	char first = input.begin[0];
	return !isascii(first) || isalpha(first) || first == '_';
}

static int is_string(string_slice input){
    return !IS_SLICE_EMPTY(input) && (input.begin[0]=='"' ||
		input.begin[0] == '\'');
}

static bool is_punctor(string_slice input, char punc) {
	return !IS_SLICE_EMPTY(input) && input.begin[0] == punc &&
		input.begin + 1 == input.end;
}

static bool is_space(string_slice input) {
	return !IS_SLICE_EMPTY(input) && isascii(input.begin[0]) &&
		isspace(input.begin[0]);
}

#endif