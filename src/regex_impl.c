#include "private/regex_impl.h"
#include <stdint.h>

#if JANSSONPATH_REGEX_ENGINE == ENGINE_system_regex
JANSSONPATH_NO_EXPORT jansson_regex_t* regex_compile(const char* pattern,
                                                     jsonpath_error_t* error) {
    static char error_msg[1024];
        jansson_regex_t ret=do_malloc(sizeof(jansson_regex_t);
	int error_code = regcomp(ret, pattern, REG_EXTENDED| REG_NOSUB);
	if (error_code) {
        regerror(error_code, ret, error_msg, 1024);
        *error = jsonpath_error_regex_compile_error(error_code, error_msg);
        return NULL;
	}
	return ret;
}

JANSSONPATH_NO_EXPORT void regex_free(jansson_regex_t* regex) {
    regfree(regex);
    do_free(regex);
}

#define regex_match(subject, regex) (!regexec((regex), (subject), 0, NULL, 0))

#elif JANSSONPATH_REGEX_ENGINE == ENGINE_PCRE2
JANSSONPATH_NO_EXPORT jansson_regex_t* regex_compile(const char* pattern,
                                                     jsonpath_error_t* error) {
    size_t offset;
    int error_code;
    static char error_msg[1024];
    jansson_regex_t* ret =
        pcre2_compile((PCRE2_SPTR8)pattern, PCRE2_ZERO_TERMINATED,
                      PCRE2_UTF | PCRE2_NO_AUTO_CAPTURE | PCRE2_EXTENDED,
                      &error_code, &offset, NULL);
    if (error_code != 100) {
        pcre2_get_error_message(error_code, (PCRE2_UCHAR8*)error_msg,
                                1024);  // why it need a buffer???
        *error = jsonpath_error_regex_compile_error(error_code, error_msg);
        return NULL;
    }
    return ret;
}

JANSSONPATH_NO_EXPORT bool regex_match(const char* subject,
                                       jansson_regex_t* regex) {
    pcre2_match_data* match_data = pcre2_match_data_create(16, NULL);
    int n = pcre2_match(regex, (PCRE2_SPTR8)subject, PCRE2_ZERO_TERMINATED, 0,
                        0, match_data, NULL);
    pcre2_match_data_free(match_data);
    assert(n != 0);   // do not use capature!
    assert(n >= -1);  // -1 for no match
    return n > 0;
}

#elif JANSSONPATH_REGEX_ENGINE == ENGINE_PCRE

JANSSONPATH_NO_EXPORT jansson_regex_t* regex_compile(const char* pattern,
	jsonpath_error_t* error) {
	int error_code;
	const char* error_msg;
	int error_offset;
	pcre* ret = pcre_compile2(pattern, PCRE_UTF8 | PCRE_NO_AUTO_CAPTURE | PCRE_EXTENDED, &error_code, &error_msg, &error_offset, NULL);
	if(error_code)
		* error = jsonpath_error_regex_compile_error(error_code, error_msg);
	return ret;
}

JANSSONPATH_NO_EXPORT bool regex_match(const char* subject,
	jansson_regex_t* regex) {
	int ovector[16];
	int ret = pcre_exec(regex, NULL, subject, (int)strlen(subject), 0, 0, ovector, 16);

	return ret>=1;
}



#endif
