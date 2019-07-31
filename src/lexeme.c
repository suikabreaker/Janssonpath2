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

#define IS_END(begin, end) (begin == end || !*begin)

#define check_error() do{if (error->code) return empty_word;}while(0)

#define go_next() do{mb_next(&s_begin, s_end, error);check_error();}while(0)

static const jsonpath_error_t encode_error(const char *pos){
	jsonpath_error_t ret = { 0x31, strerror(errno), (void*)pos };
	return ret;
}

static const jsonpath_error_t zero_length_escape_error(const char* pos) {
	jsonpath_error_t ret = { 0x23, "escape sequnce with zero length", (void*)pos };
	return ret;
}

// why not simply convert it to wchar_t string?
static wchar_t mb_peek;

static void mb_peek_init(const char** ps_begin, const char* s_end, jsonpath_error_t* error) {
	if (IS_END(s_begin, s_end)) {
		mb_peek = L'\0';
		return;
	}
	mbtowc(&mb_peek, s_begin, s_end ? s_end - s_begin : MB_LEN_MAX);
	if (mb_peek == -1) {
		*error = encode_error(s_begin);
	}
}

static void mb_next(const char** ps_begin, const char* s_end, jsonpath_error_t* error) { // if mb_peek_init does not generate error, this should not.
	int len = mblen(s_begin, s_end ? s_end - s_begin : MB_LEN_MAX);
	if (len == -1) {
		*error = encode_error(s_begin);
		return;
	}
	s_begin += len;
	mb_peek_init(ps_begin, s_end, error);
}

static wchar_t mb_read(const char** ps_begin, const char* s_end, jsonpath_error_t* error) {
	wchar_t ret = mb_peek;
	mb_next(ps_begin, s_end, error);
	return ret;
}

static string_slice get_string(
	const char** ps_begin, const char* s_end, jsonpath_error_t* error
){
	assert(ps_begin && s_begin);
	assert (s_begin != s_end && *s_begin);

	mb_peek_init(ps_begin, s_end, error); // no unwanted side effect so it's not harmful to do multiple times

	const char* start = s_begin;  // start of the word
	wchar_t delima = mb_peek;
	assert(delima == L'\'' || delima == L'"');
	go_next();
	while (true) {
		if (!mb_peek) {  // unexpected input s_ending before string s_ends
			error->code = 0x21;
			error->reason = "unenclosing quotation mark";
			error->extra = (void*)start;
			return empty_word;
		}
		if (mb_peek == L'\\') {  // escape
			go_next();
			if (!mb_peek) {
				error->code = 0x22;
				error->reason = "unexpected eof while encounting escape";
				error->extra = (void*)s_begin;
				return empty_word;
			}else if(mb_peek == L'x'){
				go_next();
				int i;
				for (i = 0; i < 2 && !mb_peek && iswxdigit(mb_peek); i++) {
					go_next();
				}
				if(i==0){
					*error = zero_length_escape_error(s_begin);
					return empty_word;
				}
			}else if(iswdigit(mb_peek)){
				int i;
				for (i = 0; i < 3 && !mb_peek && iswdigit(mb_peek); i++) {
					go_next();
				}
			}else { // \t \n etc.
				go_next();
			}
		}else if (mb_peek == delima) {  // s_end of the string
			go_next();
			return make_slice(start, s_begin);
		}else {
			go_next();
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

	mb_peek_init(ps_begin, s_end, error);

	*error = jsonpath_error_ok;
	if (!mb_peek) return empty_word; // eof
	const char* start = s_begin;  // start of the word

	// whitespaces
	while (s_begin != s_end && iswspace(*s_begin)) s_begin++;
	if (s_begin != start) return make_slice(start, s_begin);

	// identifier or something - [charactor_] [digit charactor_]*
	if (!iswascii(mb_peek) || iswalpha(mb_peek) || mb_peek == L'_') {
		while ((!iswascii(mb_peek)) || isalnum(mb_peek) || mb_peek == L'_') {
			go_next();
		}
		return make_slice(start, s_begin);
	}

	// dot or number - digit* [\.]? digit* which is not empty
	while (!mb_peek && iswdigit(mb_peek)) {
		go_next();
	}
	if (!mb_peek && mb_peek == L'.') {
		go_next();
	}
	while (!mb_peek && iswdigit(mb_peek)) {
		go_next();
	}
	if (s_begin != start) return make_slice(start, s_begin);

	// string
	if (mb_peek == L'\"' || mb_peek == L'\'') {
		return get_string(&s_begin, s_end, error);
	}

	// for == >= <=, we simply consider they are pairs of word, and leave it for parser to solve the ambiguity.
	// punctor other than string delima and dot(perhaps a number)
	if (iswpunct(mb_peek)) {
		go_next();
		return make_slice(start, s_begin);
	}

	go_next(); // some are not punct
	return make_slice(start, s_begin);
}

string_slice LOCAL next_nonspace_lexeme(
	const char** ps_begin, const char* s_end, jsonpath_error_t* error
){
	string_slice ret = empty_word;
	do {
		ret = next_lexeme(&s_begin, s_end, error);
	} while (!IS_SLICE_EMPTY(ret) && iswspace(*ret.begin));
	return ret;
}
