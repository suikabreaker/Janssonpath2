#include "private/lexeme.h"
#include <locale.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
    const char* test;
    char buffer[1024];
    if (argc < 2) {
        scanf("%[^\n]", buffer);
        test = buffer;
    } else {
        test = argv[1];
    }
    jsonpath_set_encode_recoverable(true);
    string_slice word;
    const char* test1 = test;
    jsonpath_error_t error;
    for (word = next_lexeme(&test1, NULL, &error);
         !(IS_SLICE_EMPTY(word) && !error.abort);
         word = next_lexeme(&test1, NULL, &error)) {
        if (error.abort) {
            const char* pos = error.extra;
            printf("ERROR:%s at %zd [%d]\n", error.reason, pos - test,
                   (int)(*pos));
            break;
        }
        printf("[%" FMT_SLICE "]", SLCIE_OUT(word));
    }
    puts("");
    const char* test2 = test;
    const char* test2end = test + strlen(test2);
    for (word = next_lexeme(&test2, test2end, &error);
         !(IS_SLICE_EMPTY(word) && !error.abort);
         word = next_lexeme(&test2, test2end, &error)) {
        if (error.abort) {
            const char* pos = error.extra;
            printf("ERROR:%s at %zd [%d]\n", error.reason, pos - test,
                   (int)(*pos));
            break;
        }
        printf("[%" FMT_SLICE "]", SLCIE_OUT(word));
    }
    puts("");
    const char* test3 = test;
    for (word = next_nonspace_lexeme(&test3, NULL, &error);
         !(IS_SLICE_EMPTY(word) && !error.abort);
         word = next_nonspace_lexeme(&test3, NULL, &error)) {
        if (error.abort) {
            const char* pos = error.extra;
            printf("ERROR:%s at %zd [%d]\n", error.reason, pos - test,
                   (int)(*pos));
            break;
        }
        printf("[%" FMT_SLICE "]", SLCIE_OUT(word));
    }
    puts("");
    return 0;
}
