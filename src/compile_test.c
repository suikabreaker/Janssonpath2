#include "janssonpath.h"

#include <stdio.h>
#include <locale.h>
#include <string.h>

int main(int argc, char** argv) {
	const char* test;
	char buffer[1024];
	if (argc < 2) {
		scanf("%[^\n]", buffer);
		test = buffer;
	}

	jsonpath_error_t error;
	jsonpath_t* jsonpath = jsonpath_compile_ranged(test, NULL, &error);
	puts("compiled");
	jsonpath_release(jsonpath);
	puts("released");
	return 0;
}
