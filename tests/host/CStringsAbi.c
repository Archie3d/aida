#include <stdbool.h>
#include <stdlib.h>

static _Thread_local bool failNext;
static _Thread_local size_t lastRequest;

static void* testMalloc(size_t count)
{
    lastRequest = count;
    if (failNext) {
        failNext = false;
        return NULL;
    }
    return malloc(count);
}

// Substitute only this translation unit's allocator. RunTasking also links the
// runtime archive, which supplies all the other runtime functions.
#define malloc testMalloc
#include "../../runtime/adacstrings.c"
#undef malloc

void aida_c_strings_fail_next(void)
{
    failNext = true;
}

size_t aida_c_strings_last_request(void)
{
    return lastRequest;
}

char* aida_c_strings_raw(int terminated)
{
    char* data = malloc(2);
    if (data != NULL) {
        data[0] = 'x';
        data[1] = terminated ? '\0' : 'y';
    }
    return data;
}

void aida_c_strings_release_raw(char* data)
{
    free(data);
}

void aida_c_strings_set_last(char** vector, size_t count, const char* expected, int* status)
{
    static char replacement[] = "native";
    if (count != 2 || vector[0] != expected || vector[1] != NULL) {
        *status = 0;
        return;
    }
    vector[1] = replacement;
    *status = 1;
}
