#include "janssonpath.h"

#include <stdio.h>
#include <locale.h>
#include <string.h>

#define bool_to_check(boolean) ((boolean)?'*': ' ')

static void output_result(jsonpath_result_t result){
	printf("collection:[%c], right_value:[%c], constant:[%c]\n", bool_to_check(result.is_collection), bool_to_check(result.is_right_value), bool_to_check(result.is_constant));
	json_t* root = json_array();
	json_array_append(root, result.value);
	char* out = json_dumps(root, JSON_INDENT(2) | JSON_EMBED);
	puts(out);
	free(out);
	json_decref(root);
}

int test(const char* test_path){
	int ret = 0;

	jsonpath_error_t error;
	jsonpath_t* jsonpath = jsonpath_compile_ranged(test_path, NULL, &error);
	if (error.code) {
		printf("%s: %s\n", error.abort ? "Error" : "Warning", error.reason);
		printf("At position [%zd]\n", (const char*)error.extra - test_path);
		return -1;
	}
	puts("compiled");
	jsonpath_result_t result = jsonpath_evaluate(NULL, jsonpath, NULL, &error);
	if (error.code) {
		printf("%s: %s\n", error.abort ? "Error" : "Warning", error.reason);
		printf("At position [%zd]\n", (const char*)error.extra - test_path);
		ret = -2;
		goto jsonpath_;
	}
	puts("evaluated to:");
	output_result(result);

	jsonpath_result_t result2 = jsonpath_evaluate(NULL, jsonpath, NULL, &error); // second time to check if constant fold works
	if (error.code) {
		printf("%s: %s\n", error.abort ? "Error" : "Warning", error.reason);
		printf("At position [%zd]\n", (const char*)error.extra - test_path);
		ret = -3;
		goto result1_;
	}
	puts("second time evaluated to:");
	output_result(result2);
	jsonpath_decref(result2);
result1_:
	jsonpath_decref(result);
jsonpath_:
	jsonpath_release(jsonpath);
	puts("released");
	return 0;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		char buffer[1024];
		while (scanf(" %[^\n]", buffer) > 0) {
			test(buffer);
		}
		return 0;
	}else{
		return test(argv[1]);
	}

	
}
