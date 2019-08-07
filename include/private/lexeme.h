#ifndef LEXEME_H
#define LEXEME_H
#include "common.h"
#include <wctype.h>
#include <ctype.h>

#ifndef HAVE_ISWASCII
#define iswascii isascii
#endif

void JANSSONPATH_EXPORT jsonpath_set_encode_recoverable(bool value);

string_slice JANSSONPATH_NO_EXPORT next_lexeme(
	const char** ps_begin, const char* s_end, jsonpath_error_t* error
);
string_slice JANSSONPATH_NO_EXPORT next_nonspace_lexeme(
	const char** ps_begin, const char* s_end, jsonpath_error_t* error
);

// we assume it's ASCII compatiable encoded (E.g. UTF-8)
#define is_eof IS_SLICE_EMPTY

// expect them to be inlined
#define is_number(input)(!IS_SLICE_EMPTY(input) &&\
		((isascii((input).begin[0]) && isdigit((input).begin[0])) ||\
		(((input).begin[0] == '.') && (input).end - (input).begin > 1 && ((input).begin[1] != '.')))\
)

#define is_identifier(input) (!IS_SLICE_EMPTY(input)&&(\
!isascii((input).begin[0]) || isalpha((input).begin[0]) || (input).begin[0] == '_'\
))

#define is_string(input)(!IS_SLICE_EMPTY(input) && ((input).begin[0]=='"' ||\
		(input).begin[0] == '\''))

#define is_punctor(input, punc)(!IS_SLICE_EMPTY(input) && (input).begin[0] == punc &&\
		(input).begin + 1 == (input).end)

#define is_space(input) (!IS_SLICE_EMPTY(input) && isascii((input).begin[0]) && \
		isspace((input).begin[0]))

#endif