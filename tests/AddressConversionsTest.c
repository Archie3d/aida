#include "../runtime/adart.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "address conversions check failed at line %d\n", __LINE__); abort(); \
} } while (0)

enum { kWorkerCount = 4, kIterations = 1000 };

static void checkPointer(void* pointer, int64_t alignment, AdaTaskContext* context)
{
    void* result = __ada_address_to_pointer(pointer, alignment);
    if ((uintptr_t)pointer % (uint64_t)alignment == 0) {
        CHECK(result == pointer && context->exception == NULL);
        CHECK(__ada_pointer_to_address(result) == pointer);
    } else {
        CHECK(result == NULL && context->exception == ADA_PROGRAM_ERROR);
        context->exception = NULL;
    }
}

static void* exercise(void* argument)
{
    (void)argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    _Alignas(8) unsigned char data[24] = {0};
    static const uint64_t kBits[] = {0, 1, 7, 8, INT64_MAX, (uint64_t)INT64_MAX + 1, UINT64_MAX - 7, UINT64_MAX};
    static const int64_t kInvalid[] = {0, -1, 3, INT64_MIN};
    for (int iteration = 0; iteration < kIterations; ++iteration) {
        for (int64_t alignment = 1; alignment <= 8; alignment *= 2) {
            for (unsigned i = 0; i < sizeof kBits / sizeof kBits[0]; ++i) {
                checkPointer((void*)(uintptr_t)kBits[i], alignment, &context);
            }
            for (int i = 0; i < 24; ++i) {
                checkPointer(&data[i], alignment, &context);
            }
        }
        for (unsigned i = 0; i < sizeof kInvalid / sizeof kInvalid[0]; ++i) {
            CHECK(__ada_address_to_pointer(data, kInvalid[i]) == NULL);
            CHECK(context.exception == ADA_PROGRAM_ERROR);
            context.exception = NULL;
        }
        unsigned char* pointer = __ada_address_to_pointer(&data[8], 8);
        *pointer = 42;
        CHECK(data[8] == 42 && __ada_pointer_to_address(pointer) == &data[8]);
    }
    CHECK(context.exception == NULL && __ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    AdaThread workers[kWorkerCount];
    for (int i = 0; i < kWorkerCount; ++i) {
        CHECK(__ada_thread_create(&workers[i], exercise, NULL) == 0);
    }
    for (int i = 0; i < kWorkerCount; ++i) {
        CHECK(__ada_thread_join(workers[i], NULL) == 0);
    }
    return 0;
}
