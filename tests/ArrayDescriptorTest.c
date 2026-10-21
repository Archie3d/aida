#include "../runtime/adart.h"
#include "../runtime/adathread.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "array descriptor check failed at line %d\n", __LINE__); abort(); \
} } while (0)

static void expectConstraint(AdaTaskContext* context)
{
    CHECK(context->exception == ADA_CONSTRAINT_ERROR);
    context->exception = NULL;
}

static void expectStorage(AdaTaskContext* context)
{
    CHECK(context->exception == ADA_STORAGE_ERROR);
    context->exception = NULL;
}

static void* exercise(void* argument)
{
    (void)argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    for (int i = 0; i < 100; ++i) {
        CHECK(sizeof(AdaArrayDimension) == 32 && sizeof(AdaArrayResult) == 48);
        CHECK(adaArrayLocalSize(3) == 104 && adaArrayResultSize(3) == 112);
        CHECK(__ada_array_length(INT64_MIN, INT64_MIN + 2) == 3);
        CHECK(__ada_array_length(INT64_MAX - 2, INT64_MAX) == 3);
        CHECK(__ada_array_length(INT64_MAX, INT64_MIN) == 0);
        CHECK(__ada_array_length(INT_MIN, INT_MAX) == UINT64_C(4294967296));
        CHECK(__ada_array_length(INT64_MIN, -2) == INT64_MAX);
        CHECK(__ada_array_size(INT_MIN, INT_MAX, 0) == 0);
        CHECK(__ada_array_size(INT_MIN, INT_MAX, 4) == INT64_C(17179869184));
        CHECK(__ada_array_offset(INT64_MIN + 2, INT64_MIN, INT64_MIN + 2, 12) == 24);
        CHECK(__ada_array_offset(INT64_MAX, INT64_MAX - 2, INT64_MAX, 12) == 24);
        CHECK(__ada_array_offset(INT64_MAX, INT64_MIN, INT64_MAX, 0) == 0);
        CHECK(__ada_array_slice_offset(INT64_MAX, INT64_MIN, 1, 2, 4) == 0);
        CHECK(context.exception == NULL);
        __ada_array_length(INT64_MIN, INT64_MAX);
        expectStorage(&context);
        __ada_array_length(INT64_MIN, -1);
        expectStorage(&context);
        __ada_array_size(1, INT64_MAX, 2);
        expectStorage(&context);
        __ada_array_size(1, 0, -1);
        expectStorage(&context);
        __ada_array_offset(INT64_MAX, INT64_MIN, INT64_MAX, 1);
        expectStorage(&context);
        __ada_array_offset(4, 1, 3, 4);
        expectConstraint(&context);
        __ada_array_slice_offset(1, 4, 1, 3, 4);
        expectConstraint(&context);
        __ada_array_narrow(INT64_C(4294967296));
        expectConstraint(&context);
        CHECK(__ada_array_narrow(INT_MAX) == INT_MAX);
        void* owner = NULL;
        int* values = __ada_array_local(&owner, INT64_MAX - 2, INT64_MAX, sizeof(int));
        CHECK(values != NULL);
        values[2] = 91;
        AdaArrayResult result = {0};
        __ada_array_result(&result, values, INT64_MAX - 2, INT64_MAX, sizeof(int));
        CHECK(result.first == INT64_MAX - 2 && result.last == INT64_MAX);
        CHECK(result.length == 3 && result.stride == sizeof(int) && result.size == 3 * sizeof(int));
        CHECK(((int*)result.data)[2] == 91);
        __ada_array_adopt(&owner, result.data);
        __ada_array_release(&owner);
        CHECK(owner == NULL);
        // Allocation must still work while an exception is being captured.
        context.exception = ADA_CONSTRAINT_ERROR;
        CHECK(__ada_array_local(&owner, 1, 5, 1) != NULL);
        expectConstraint(&context);
        __ada_array_release(&owner);
    }
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    AdaThread workers[4];
    for (int i = 0; i < 4; ++i) {
        CHECK(__ada_thread_create(&workers[i], exercise, NULL) == 0);
    }
    for (int i = 0; i < 4; ++i) {
        CHECK(__ada_thread_join(workers[i], NULL) == 0);
    }
    CHECK(context.exception == NULL && __ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return 0;
}
