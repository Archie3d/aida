#include "../runtime/adart.h"
#include "../runtime/adathread.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "enumeration runtime check failed at line %d\n", __LINE__); abort(); \
} } while (0)

static void expectConstraint(AdaTaskContext* context)
{
    CHECK(context->exception == ADA_CONSTRAINT_ERROR);
    context->exception = NULL;
}

static void* exercise(void* argument)
{
    (void)argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    const int32_t endpoints[] = { INT32_MIN, -128, -1, 0, 127, 255, 65535, INT32_MAX };
    for (int i = 0; i < 8; ++i) {
        CHECK(__ada_enum_code(i, endpoints, 8) == endpoints[i]);
        CHECK(__ada_enum_position(endpoints[i], endpoints, 8) == i);
        CHECK(context.exception == NULL);
    }
    int32_t codes[1000];
    for (int i = 0; i < 1000; ++i) { codes[i] = i * 5 - 2000; }
    for (int i = 0; i < 1000; ++i) {
        CHECK(__ada_enum_position(codes[i], codes, 1000) == i);
        __ada_enum_position(codes[i] + 1, codes, 1000);
        expectConstraint(&context);
    }
    __ada_enum_code(-1, codes, 1000);
    expectConstraint(&context);
    __ada_enum_code(1000, codes, 1000);
    expectConstraint(&context);
    __ada_enum_position(INT_MIN, codes, 1000);
    expectConstraint(&context);
    __ada_enum_position(INT_MAX, codes, 1000);
    expectConstraint(&context);
    __ada_enum_position(0, NULL, 0);
    expectConstraint(&context);
    char image[8];
    const char* names[] = { "'a'", "'A'", "name" };
    CHECK(__ada_image_enum(image, sizeof(image), 0, names, 3) == 3);
    CHECK(strcmp(image, "'a'") == 0);
    CHECK(__ada_value_enum("'a'", 3, names, 3) == 0);
    CHECK(__ada_value_enum("'A'", 3, names, 3) == 1);
    CHECK(__ada_value_enum("NaMe", 4, names, 3) == 2);
    CHECK(context.exception == NULL);
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    AdaThread threads[4];
    for (int i = 0; i < 4; ++i) {
        CHECK(__ada_thread_create(&threads[i], exercise, NULL) == 0);
    }
    for (int i = 0; i < 4; ++i) {
        CHECK(__ada_thread_join(threads[i], NULL) == 0);
    }
    return 0;
}
