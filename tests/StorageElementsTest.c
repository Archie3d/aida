#include "../runtime/adart.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "storage elements check failed at line %d\n", __LINE__); abort(); \
} } while (0)

enum { kWorkerCount = 4, kIterations = 20000 };

static void checkPair(uint64_t address, int64_t offset, AdaTaskContext* context)
{
    void* pointer = __ada_storage_to_address(address);
    CHECK(__ada_storage_to_integer(pointer) == address);
    // A widened mathematical oracle covers carry, borrow and the signed minimum.
    __int128 modulus = (__int128)1 << 64;
    __int128 sum = (__int128)address + offset;
    uint64_t expected = (uint64_t)((sum + modulus) % modulus);
    CHECK(__ada_storage_to_integer(__ada_storage_add(pointer, offset)) == expected);
    CHECK(__ada_storage_offset_add(offset, pointer) == __ada_storage_add(pointer, offset));
    __int128 difference = (__int128)address - offset;
    expected = (uint64_t)((difference + modulus) % modulus);
    CHECK(__ada_storage_to_integer(__ada_storage_subtract(pointer, offset)) == expected);
    void* other = __ada_storage_to_address((uint64_t)offset);
    __int128 signedDifference = ((__int128)address - (uint64_t)offset + modulus) % modulus;
    if (signedDifference > INT64_MAX) { signedDifference -= modulus; }
    CHECK(__ada_storage_difference(pointer, other) == signedDifference);
    if (offset != 0) {
        __int128 remainder = (__int128)address % offset;
        if (remainder != 0 && offset < 0) { remainder += offset; }
        CHECK(__ada_storage_mod(pointer, offset) == remainder);
        CHECK(context->exception == NULL);
    } else {
        CHECK(__ada_storage_mod(pointer, 0) == 0);
        CHECK(context->exception == ADA_PROGRAM_ERROR);
        context->exception = NULL;
    }
}

static void* exercise(void* argument)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    static const uint64_t kAddresses[] = {0, 1, 7, INT64_MAX, (uint64_t)INT64_MAX + 1, UINT64_MAX - 1, UINT64_MAX};
    static const int64_t kOffsets[] = {INT64_MIN, INT64_MIN + 1, -7, -1, 0, 1, 7, INT64_MAX};
    for (unsigned i = 0; i < sizeof kAddresses / sizeof kAddresses[0]; ++i) {
        for (unsigned j = 0; j < sizeof kOffsets / sizeof kOffsets[0]; ++j) {
            checkPair(kAddresses[i], kOffsets[j], &context);
        }
    }
    uint64_t state = (uintptr_t)argument + 1;
    for (int i = 0; i < kIterations; ++i) {
        state = state * UINT64_C(6364136223846793005) + 1;
        uint64_t address = state;
        state = state * UINT64_C(6364136223846793005) + 1;
        int64_t offset = state <= INT64_MAX ? (int64_t)state : -1 - (int64_t)(UINT64_MAX - state);
        checkPair(address, offset, &context);
    }
    unsigned char data[16] = {0};
    void* start = __ada_storage_to_address((uintptr_t)data);
    for (int i = 0; i < 16; ++i) {
        unsigned char* element = __ada_storage_add(start, i);
        CHECK(element == &data[i]);
        *element = (unsigned char)i;
        CHECK(__ada_storage_difference(element, start) == i);
        CHECK(__ada_storage_mod(element, 1) == 0);
    }
    CHECK(data[15] == 15 && context.exception == NULL);
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    AdaThread workers[kWorkerCount];
    for (int i = 0; i < kWorkerCount; ++i) {
        CHECK(__ada_thread_create(&workers[i], exercise, (void*)(uintptr_t)i) == 0);
    }
    for (int i = 0; i < kWorkerCount; ++i) {
        CHECK(__ada_thread_join(workers[i], NULL) == 0);
    }
    puts("storage elements runtime ok");
    return 0;
}
