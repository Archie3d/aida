#include "../runtime/adart.h"
#include "../runtime/adathread.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "concatenation check failed at line %d\n", __LINE__); abort(); \
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
        AdaArrayDimension shape;
        CHECK(__ada_array_concat_shape(&shape, 5, 2, 9, 10, 2, 1, 100, 0, 8) == 32);
        CHECK(shape.first == 5 && shape.last == 8 && shape.length == 4 && shape.stride == 8);
        CHECK(__ada_array_concat_shape(&shape, 7, 0, 12, 2, 0, 1, 100, 0, 8) == 0);
        CHECK(shape.first == 12 && shape.last == 2 && shape.length == 0);
        CHECK(__ada_array_concat_shape(&shape, 7, 0, 9, 10, 2, 1, 100, 0, 8) == 16);
        CHECK(shape.first == 9 && shape.last == 10);
        CHECK(__ada_array_concat_shape(&shape, 7, 2, 1, 1, 1, 5, 7, 1, 4) == 12);
        CHECK(shape.first == 5 && shape.last == 7);
        CHECK(__ada_array_concat_shape(&shape, INT64_MIN, 1, 1, 0, 0, INT64_MIN, INT64_MAX, 0, 4) == 4);
        CHECK(shape.first == INT64_MIN && shape.last == INT64_MIN);
        CHECK(__ada_array_concat_shape(&shape, INT64_MAX - 1, 1, 1, 1, 1, INT64_MIN, INT64_MAX, 0, 4) == 8);
        CHECK(shape.first == INT64_MAX - 1 && shape.last == INT64_MAX);
        CHECK(__ada_array_concat_shape(&shape, 0, INT64_MAX - 1, 0, 0, 1, 0, INT64_MAX, 0, 0) == 0);
        CHECK(shape.length == INT64_MAX);
        CHECK(context.exception == NULL);
        __ada_array_concat_shape(&shape, 0, INT64_MAX, 0, 0, 1, 0, INT64_MAX, 0, 0);
        expectStorage(&context);
        __ada_array_concat_shape(&shape, 0, 2, 0, 0, 1, 0, INT64_MAX, 0, INT64_MAX);
        expectStorage(&context);
        __ada_array_concat_shape(&shape, INT64_MAX, 1, 0, 0, 1, INT64_MIN, INT64_MAX, 0, 1);
        expectConstraint(&context);
        __ada_array_concat_shape(&shape, 3, 2, 1, 1, 1, 1, 4, 0, 1);
        expectConstraint(&context);
        __ada_array_concat_shape(&shape, 5, 1, 1, 1, 1, 5, 4, 1, 1);
        expectConstraint(&context);
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
