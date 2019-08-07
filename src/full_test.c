#include "janssonpath.h"

#include <locale.h>
#include <stdio.h>
#include <string.h>

#define bool_to_check(boolean) ((boolean) ? '*' : ' ')

static void output_result(jsonpath_result_t result) {
    if (result.value) {
        if (result.is_collection && json_array_size(result.value))
            printf("result[0].refcount= %zd\n",
                   json_array_get(result.value, 0)->refcount);
        printf("result.refcount= %zd\n", result.value->refcount);
    }
    printf("collection[%c] right_value[%c] constant[%c]\n",
           bool_to_check(result.is_collection),
           bool_to_check(result.is_right_value),
           bool_to_check(result.is_constant));
    json_t* root = json_array();
    json_array_append(root, result.value);
    char* out = json_dumps(root, JSON_INDENT(2) | JSON_EMBED);
    if (out) {
        puts(out);
        free(out);
    }
    json_decref(root);
}

static int test(json_t* json, const char* test_path) {
    int ret = 0;

    jsonpath_error_t error;
    jsonpath_t* jsonpath = jsonpath_compile_ranged(test_path, NULL, &error);
    if (error.code) {
        printf("%s: %s\n", error.abort ? "Error" : "Warning", error.reason);
        printf("At position [%zd]\n", (const char*)error.extra - test_path);
        if (error.abort) return -1;
    }
    puts("compiled");
    jsonpath_result_t result = jsonpath_evaluate(json, jsonpath, NULL, &error);
    if (error.code) {
        printf("%s: %s\n", error.abort ? "Error" : "Warning", error.reason);
        printf("At position [%zd]\n", (const char*)error.extra - test_path);
        if (error.abort) {
            ret = -2;
            goto jsonpath_;
        }
    }
    puts("evaluated to:");
    output_result(result);

    jsonpath_result_t result2 = jsonpath_evaluate(
        json, jsonpath, NULL,
        &error);  // second time to check if constant fold works
    if (error.code) {
        printf("%s: %s\n", error.abort ? "Error" : "Warning", error.reason);
        printf("At position [%zd]\n", (const char*)error.extra - test_path);
        if (error.abort) {
            ret = -3;
            goto result1_;
        }
    }
    puts(
        "second time evaluated to(note that first result is not released "
        "yet):");
    output_result(result2);

    jsonpath_decref(result2);
result1_:
    jsonpath_decref(result);
jsonpath_:
    jsonpath_release(jsonpath);
    puts("released");
    return ret;
}

int main(int argc, char** argv) {
    json_error_t error;
    if (argc < 3) {
        char buffer[1024];
        scanf("%s", buffer);
        json_t* json =
            json_load_file(buffer, JSON_DECODE_ANY | JSON_ALLOW_NUL, &error);
        while (scanf(" %[^\n]", buffer) > 0) {
            test(json, buffer);
        }
        json_decref(json);
        return 0;
    } else {
        json_t* json =
            json_load_file(argv[1], JSON_DECODE_ANY | JSON_ALLOW_NUL, &error);
        int ret = test(json, argv[2]);
        json_decref(json);
        return ret;
    }
}
