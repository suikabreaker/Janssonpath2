#ifndef REGEX_IMPL_H
#define REGEX_IMPL_H

#include "janssonpath_conf.h"
#include "janssonpath_error.h"
#include "private/error.h"
#include "private/jansson_memory.h"

// =~ is sub-match. If you want to make a full match, consider using ^ $ in
// single line case

#if JANSSONPATH_REGEX_ENGINE == ENGINE_system_regex

#include "regex.h"
typedef regex_t jansson_regex_t;
JANSSONPATH_NO_EXPORT jansson_regex_t* regex_compile(const char* pattern,
                                                     jsonpath_error_t* error);
JANSSONPATH_NO_EXPORT void regex_free(jansson_regex_t* regex);
#define regex_match(subject, regex) (!regexec((regex), (subject), 0, NULL, 0))

#elif JANSSONPATH_REGEX_ENGINE == ENGINE_PCRE2

#define PCRE2_CODE_UNIT_WIDTH 8
#include "pcre2.h"
typedef pcre2_code jansson_regex_t;
JANSSONPATH_NO_EXPORT jansson_regex_t* regex_compile(const char* pattern,
                                                     jsonpath_error_t* error);
#define regex_free pcre2_code_free
JANSSONPATH_NO_EXPORT bool regex_match(const char* subject,
                                       jansson_regex_t* regex);

#endif
#endif
