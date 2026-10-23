#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__APPLE__)
#include <malloc/malloc.h>
#elif defined(__linux__) || defined(_WIN32)
#include <malloc.h>
#endif

int64_t __test_size_value(const void* address, int bytes, int isSigned)
{
    if (bytes == 1) {
        uint8_t value;
        memcpy(&value, address, sizeof(value));
        return isSigned ? (int8_t)value : value;
    }
    if (bytes == 2) {
        uint16_t value;
        memcpy(&value, address, sizeof(value));
        return isSigned ? (int16_t)value : value;
    }
    if (bytes == 4) {
        int32_t value;
        memcpy(&value, address, sizeof(value));
        return value;
    }
    int64_t value;
    memcpy(&value, address, sizeof(value));
    return value;
}

int64_t __test_size_offset(const void* first, const void* second)
{
    return (const unsigned char*)second - (const unsigned char*)first;
}

void __test_size_touch_roomy(void* address)
{
    int32_t value = 77;
    memcpy(address, &value, sizeof(value));
    ((unsigned char*)address)[31] = 90;
}

int __test_size_touch_derived(void* address)
{
    ((unsigned char*)address)[63] = 90;
    return 1;
}

void __test_size_set_long(void* address)
{
    int64_t value = -99;
    memcpy(address, &value, sizeof(value));
}

void __test_size_bad_long(void* address)
{
    int64_t value = 1000;
    memcpy(address, &value, sizeof(value));
}

void __test_size_touch(void* address, int64_t count)
{
    /* Callers pass the start of an allocated block. Check the real
       capacity before probing padding, so undersized allocations fail even
       when the out-of-bounds byte happens to remain mapped. */
#if defined(__APPLE__)
    size_t capacity = malloc_size(address);
#elif defined(__linux__)
    size_t capacity = malloc_usable_size(address);
#elif defined(_WIN32)
    size_t capacity = _msize(address);
#else
    size_t capacity = (size_t)count;
#endif
    if (count <= 0 || (uint64_t)count > capacity) {
        fprintf(stderr, "Size storage probe needs %lld bytes, allocation has %zu\n",
            (long long)count, capacity);
        abort();
    }
    ((unsigned char*)address)[count - 1] = 90;
}
