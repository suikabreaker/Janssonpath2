#include "private/regex_impl.h"
#include <stdint.h>

#if JANSSONPATH_REGEX_ENGINE == ENGINE_system_regex
JANSSONPATH_NO_EXPORT jansson_regex_t* regex_compile(const char* pattern,
                                                     jsonpath_error_t* error) {
    static char error_msg[1024];
        jansson_regex_t ret=do_malloc(sizeof(jansson_regex_t);
	int errorcode = regcomp(ret, pattern, REG_EXTENDED| REG_NOSUB);
	if (errorcode) {
        regerror(errorcode, ret, error_msg, 1024);
        *error = jsonpath_error_regex_compile_error(errorcode, error_msg);
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
    int errorcode;
    static char error_msg[1024];
    jansson_regex_t* ret =
        pcre2_compile((PCRE2_SPTR8)pattern, PCRE2_ZERO_TERMINATED,
                      PCRE2_UTF | PCRE2_NO_AUTO_CAPTURE | PCRE2_EXTENDED,
                      &errorcode, &offset, NULL);
    if (errorcode != 100) {
        pcre2_get_error_message(errorcode, (PCRE2_UCHAR8*)error_msg,
                                1024);  // why it need a buffer???
        *error = jsonpath_error_regex_compile_error(errorcode, error_msg);
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

#endif
