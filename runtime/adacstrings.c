#include <stdint.h>
#include <stdlib.h>
#include <string.h>

size_t __ada_c_strings_bounded_length(const char* data, size_t limit)
{
    size_t length = 0;
    while (length < limit && data[length] != '\0') {
        ++length;
    }
    return length;
}

char* __ada_c_strings_allocate_copy(const char* data, size_t count)
{
    size_t length = __ada_c_strings_bounded_length(data, count);
    if (length == SIZE_MAX) {
        return NULL;
    }
    char* result = malloc(length + 1);
    if (result != NULL) {
        if (length != 0) {
            memcpy(result, data, length);
        }
        result[length] = '\0';
    }
    return result;
}

void __ada_c_strings_copy_at(char* target, size_t offset, const char* data, size_t count)
{
    if (count != 0) {
        memmove(target + offset, data, count);
    }
}
