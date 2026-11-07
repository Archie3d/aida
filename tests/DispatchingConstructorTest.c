#include "../runtime/adart.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "constructor check failed at line %d\n", __LINE__); abort(); \
} } while (0)

enum { kWorkers = 4 };
static AdaTag root = { .name = "CONSTRUCTOR.ROOT" };
static AdaTag concrete = { .name = "CONSTRUCTOR.CONCRETE", .parent = &root };
static AdaTag abstract = { .name = "CONSTRUCTOR.ABSTRACT", .parent = &root, .isAbstract = 1 };

static void check(const void* tag, const void* ancestor, const AdaMaster* instance, int valid)
{
    __ada_tag_check_constructor(tag, ancestor, instance);
    CHECK(__ada_exception == (valid ? NULL : &__ada_exc_tag_error));
    __ada_exception = NULL;
}

static void* exercise(void* argument)
{
    const AdaTag* foreign = argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    AdaMaster outer, inner;
    __ada_master_enter(&context, &outer, ADA_MASTER_SUBPROGRAM);
    CHECK(__ada_constructor_master() == &outer);
    AdaTag* local = __ada_tag_create(&concrete, &root, &outer);
    CHECK(local != NULL && __ada_exception == NULL);
    check(local, &root, &outer, 1);
    check(local, &root, NULL, 0);
    check(foreign, &root, &outer, 0);
    check(&concrete, &root, &outer, 1);
    check(&abstract, &root, &outer, 0);
    check(NULL, &root, &outer, 0);
    check(&root, &concrete, &outer, 0);
    __ada_master_enter(&context, &inner, ADA_MASTER_BLOCK);
    AdaTag* nested = __ada_tag_create(&concrete, &root, &inner);
    CHECK(nested != NULL && __ada_exception == NULL);
    check(nested, &root, &outer, 0);
    check(nested, &root, &inner, 1);
    check(local, &root, &inner, 1);
    __ada_master_leave(&context, &inner);
    __ada_master_leave(&context, &outer);
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    __ada_tag_register(&root);
    __ada_tag_register(&concrete);
    __ada_tag_register(&abstract);
    AdaMaster mainMaster;
    __ada_master_enter(__ada_task_context(), &mainMaster, ADA_MASTER_SUBPROGRAM);
    AdaTag* foreign = __ada_tag_create(&concrete, &root, &mainMaster);
    CHECK(foreign != NULL && __ada_exception == NULL);
    AdaThread workers[kWorkers];
    for (int i = 0; i < kWorkers; ++i) {
        CHECK(__ada_thread_create(&workers[i], exercise, foreign) == 0);
    }
    for (int i = 0; i < kWorkers; ++i) { CHECK(__ada_thread_join(workers[i], NULL) == 0); }
    check(foreign, &root, &mainMaster, 1);
    __ada_master_leave(__ada_task_context(), &mainMaster);
    CHECK(__ada_exception == NULL);
    return 0;
}
