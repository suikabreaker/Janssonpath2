#include <stdio.h>
#include <locale.h>
#include <string.h>
#include "private/lexeme.h"

int main(int argc, char**argv){
	setlocale(LC_ALL, "chs_china.936"); // windows console default to GBK
	const char* test;
	char buffer[1024];
	if (argc < 2) {
		scanf("%[^\n]", buffer);
		test = buffer;
	}
	string_slice word;
	const char* test1 = test;
	jsonpath_error_t error;
	for (word = next_lexeme(&test1, NULL, &error); !(IS_SLICE_EMPTY(word) && !error.code); word = next_lexeme(&test1, NULL, &error)) {
		if(error.code){
			const char* pos = error.extra;
			printf("ERROR:%s at %zd [%d]\n", error.reason, pos - test, (int)(*pos));
			break;
		}
		printf("[%" FMT_SLICE "]", SLCIE_OUT(word));
	}
	puts("");
	const char* test2 = test;
	const char* test2end = test + strlen(test2);
	for (word = next_lexeme(&test2, test2end, &error); !(IS_SLICE_EMPTY(word) && !error.code); word = next_lexeme(&test2, test2end, &error)) {
		if (error.code) {
			const char* pos = error.extra;
			printf("ERROR:%s at %zd [%d]\n", error.reason, pos - test, (int)(*pos));
			break;
		}
		printf("[%" FMT_SLICE "]", SLCIE_OUT(word));
	}
	puts("");
	const char* test3 = test;
	for (word = next_nonspace_lexeme(&test3, NULL, &error); !(IS_SLICE_EMPTY(word) && !error.code); word = next_nonspace_lexeme(&test3, NULL, &error)) {
		if (error.code) {
			const char* pos = error.extra;
			printf("ERROR:%s at %zd [%d]\n", error.reason, pos - test, (int)(*pos));
			break;
		}
		printf("[%" FMT_SLICE "]", SLCIE_OUT(word));
	}
	puts("");
	return 0;
}
