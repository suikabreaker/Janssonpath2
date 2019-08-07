#ifndef REGEX_IMPL_H
#define REGEX_IMPL_H

#include "janssonpath_conf.h"
#include "janssonpath_error.h"
#include "private/jansson_memory.h"
#include <stdint.h>

// =~ is sub-match. If you want to make a full match, consider using ^ $ in single line case

#if JANSSONPATH_REGEX_ENGINE == ENGINE_system_regex
#include "regex.h"

typedef regex_t jansson_regex_t;
static jansson_regex_t* regex_compile(const char* pattern, jsonpath_error_t* error){
	static char error_msg[1024];
	jansson_regex_t ret=do_malloc(sizeof(jansson_regex_t);
	int errorcode = regcomp(ret, pattern, REG_EXTENDED| REG_NOSUB);
	if (errorcode) {
		regerror(errorcode, ret, error_msg, 1024);
		error->abort = true;
		error->code = 0xf0000000u | (unsigned)errorcode;
		error->reason = error_msg;
		error->extra = NULL;
		return NULL;
	}
	return ret;
}

static void regex_free(jansson_regex_t* regex) {
	regfree(regex);
	do_free(regex);
}

#define regex_match(subject, regex) (!regexec((regex), (subject), 0, NULL, 0))

#elif JANSSONPATH_REGEX_ENGINE == ENGINE_PCRE2
#define PCRE2_CODE_UNIT_WIDTH 8
#include "pcre2.h"

typedef pcre2_code jansson_regex_t;
static jansson_regex_t* regex_compile(const char* pattern, jsonpath_error_t* error) {
	size_t offset;
	int errorcode;
	static char error_msg[1024]; 
	jansson_regex_t* ret = pcre2_compile((PCRE2_SPTR8)pattern, PCRE2_ZERO_TERMINATED, PCRE2_UTF | PCRE2_NO_AUTO_CAPTURE | PCRE2_EXTENDED, &errorcode, &offset, NULL);
	if(errorcode!=100){
		pcre2_get_error_message(errorcode, (PCRE2_UCHAR8*)error_msg, 1024); // why it need a buffer???
		error->abort = true;
		error->code = 0xf0000000u | (unsigned)errorcode;
		error->reason = error_msg;
		error->extra = (void*)(intptr_t)offset;
		return NULL;
	}
	return ret;
}

#define regex_free pcre2_code_free

static bool regex_match(const char* subject, jansson_regex_t* regex) {
	pcre2_match_data* match_data= pcre2_match_data_create(16, NULL);
	int n = pcre2_match(regex, (PCRE2_SPTR8)subject, PCRE2_ZERO_TERMINATED, 0, 0, match_data, NULL);
	pcre2_match_data_free(match_data);
	assert(n != 0); // do not use capature!
	assert(n >= -1); // -1 for no match
	return n > 0;
}

#endif
#endif
