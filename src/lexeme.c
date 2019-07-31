#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "private/common.h"
#include "private/lexeme.h"

static const string_slice empty_word = { NULL, NULL };

static string_slice make_slice(const char* s_begin, const char* s_end) {
	string_slice ret = { s_begin,s_end };
	return ret;
}

#define s_begin (*ps_begin)

static void mb_next(const char** ps_begin, const char* s_end, jsonpath_error_t* error) {
	int len = mblen(s_begin, s_end ? s_end - s_begin : INT_MAX);
	if (len <= 0) {
		error->code = 0x31;
		error->reason = strerror(errno);
		error->extra = (void*)s_begin;
		return;
	}
	s_begin += len;
}

static string_slice get_string(
	const char** ps_begin, const char* s_end, jsonpath_error_t* error
){
	assert(ps_begin && s_begin);
	assert (s_begin != s_end && *s_begin);

	const char* start = s_begin;  // start of the word
	char delima = *s_begin;
	assert(delima == '\'' || delima == '"');
	s_begin++;
	while (true) {
		if (s_begin == s_end ||
			!*s_begin) {  // unexpected input s_ending before string s_ends
			error->code = 0x21;
			error->reason = "unenclosing quotation mark";
			error->extra = (void*)start;
			return empty_word;
		}else if (*s_begin == '\\') {  // escape
			s_begin++;
			if (!*s_begin) {
				error->code = 0x22;
				error->reason = "unexpected eof while encounting escape";
				error->extra = (void*)(s_begin - 1);
				return empty_word;
			}else if(*s_begin=='x'){
				s_begin++;
				int i;
				for (i = 0; i < 2 && s_begin != s_end && *s_begin && isascii(*s_begin) && isxdigit(*s_begin); i++);
				if(i==0){
					error->code = 0x23;
					error->reason = "zero length escape sequnce";
					error->extra = (void*)(s_begin - 2);
					return empty_word;
				}
			}else if(isascii(*s_begin) && isdigit(*s_begin)){
				int i;
				for (i = 0; i < 3 && s_begin != s_end && *s_begin && isascii(*s_begin) && isxdigit(*s_begin); i++);
				if (i == 0) {
					error->code = 0x23;
					error->reason = "zero length escape sequnce";
					error->extra = (void*)(s_begin - 2);
					return empty_word;
				}
			}else { // \t \n etc.
				s_begin++;
			}
		}else if (*s_begin == delima) {  // s_end of the string
			s_begin++;
			return make_slice(start, s_begin);
		}else {
			mb_next(&s_begin, s_end, error);
			if (error->code) return empty_word;
		}
	}
}

// note that continuous spaces are considered single word
// any punctor charactors are themself a word of length 1
string_slice LOCAL next_lexeme(
	const char** ps_begin, const char* s_end, jsonpath_error_t *error
) {
	if(!(ps_begin && s_begin)){
		error->code = 0x11;
		error->reason = "ps_begin is NULL or pointing to NULL";
		error->extra = NULL;
		return empty_word;
	}
	*error = jsonpath_error_ok;
	if (s_begin == s_end || !*s_begin) return empty_word; // eof
	const char* start = s_begin;  // start of the word

	// whitespaces
	while (s_begin != s_end && isascii(*s_begin) && isspace(*s_begin)) s_begin++;
	if (s_begin != start) return make_slice(start, s_begin);

	// identifier or something - [charactor_] [digit charactor_]*
	if (!isascii(*s_begin) || isalpha(*s_begin) || *s_begin == '_') {
		while ((!isascii(*s_begin)) || isalnum(*s_begin) || *s_begin == '_') {
			mb_next(&s_begin, s_end, error);
			if (error->code) return empty_word;
		}
		return make_slice(start, s_begin);
	}

	// dot or number - digit* [\.]? digit* which is not empty
	while (s_begin != s_end && isascii(*s_begin) && isdigit(*s_begin)) s_begin++;
	if (s_begin != s_end && (*s_begin) == '.') s_begin++;
	while (s_begin != s_end && isascii(*s_begin) && isdigit(*s_begin)) s_begin++;
	if (s_begin != start) return make_slice(start, s_begin);

	// string
	if (*s_begin == '\"' || *s_begin == '\'') {
		return get_string(&s_begin, s_end, error);
	}

	// for == >= <=, we simply consider they are pairs of word, and leave it for parser to solve the ambiguity.
	// punctor other than string delima and dot(perhaps a number)
	if (isascii(*s_begin) && ispunct(*s_begin)) {
		s_begin++;
		return make_slice(start, s_begin);
	}

	error->code = 0x12;
	error->reason = "unknown type of word";
	error->extra = (void*)start;
	++s_begin;
	return make_slice(start, s_begin);
}

string_slice LOCAL next_nonspace_lexeme(
	const char** ps_begin, const char* s_end, jsonpath_error_t* error
){
	string_slice ret = empty_word;
	do {
		ret = next_lexeme(&s_begin, s_end, error);
	} while (!IS_SLICE_EMPTY(ret) && isascii(*ret.begin) && isspace(*ret.begin));
	return ret;
}
