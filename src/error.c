#include "private/error.h"

jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_ok = {false, 0, "OK",
                                                            NULL};

jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_unknown = {
    true, 1, "unknown", NULL};

jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_nullptr = {
    true, 0x11, "ps_begin is NULL or pointing to NULL", NULL};

jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_to_array_non_collection =
    {true, 0xa3, "to_array(&) expecting collection oprand", NULL};
jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_from_array_collection = {
    true, 0xa4, "to_array(&) expecting collection oprand", NULL};

// 0x600000000 for grammar error
jsonpath_error_t JANSSONPATH_NO_EXPORT
json_error_unmatched_bracked(const char* position) {
    jsonpath_error_t ret = {true, 0x600000001u, "unmacthed bracket",
                            (void*)position};
    return ret;
}
jsonpath_error_t JANSSONPATH_NO_EXPORT
json_error_expecting_index(const char* position) {
    jsonpath_error_t ret = {true, 0x600000002u,
                            "expecting simple index(*/#/identifier)",
                            (void*)position};
    return ret;
}

// 0x200000000 for lexical error
JANSSONPATH_NO_EXPORT jsonpath_error_t
jsonpath_error_eof_escape(const char* position) {
    jsonpath_error_t ret = {true, 0x200000002u,
                            "unexpected eof while in escape sequence",
                            (void*)position};
    return ret;
}

JANSSONPATH_NO_EXPORT jsonpath_error_t
jsonpath_error_zero_length_escape(const char* position) {
    jsonpath_error_t ret = {true, 0x200000003u,
                            "escape sequnce with length of zero",
                            (void*)position};
    return ret;
}

// 0xf0000000 for regex error
JANSSONPATH_NO_EXPORT jsonpath_error_t
jsonpath_error_regex_compile_error(unsigned errorcode, const char* error_msg) {
    jsonpath_error_t ret = {true, 0xf000000u | errorcode, error_msg, NULL};
    return ret;
}

JANSSONPATH_NO_EXPORT jsonpath_error_t
jsonpath_error_function_not_found(const char* function_name) {
    jsonpath_error_t ret = {true, 0x800000001u,
                            "Function not found in the table",
                            (void*)function_name};
    return ret;
}