#include "../runtime/adart.h"
#include "../runtime/adathread.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "task runtime check failed at line %d\n", __LINE__); abort(); \
} } while (0)

static atomic_int completed;

static void successfulTask(void* argument)
{
    (void)argument;
    AdaTaskContext* context = __ada_task_context();
    CHECK(context != &__ada_main_context && context->taskSelf != NULL);
    AdaMaster master;
    __ada_master_enter(context, &master, ADA_MASTER_SUBPROGRAM);
    __ada_master_activate(context, &master);
    __ada_task_activated(context);
    atomic_fetch_add(&completed, 1);
    __ada_master_await(context, &master);
    __ada_master_leave(context, &master);
}

static void failedTask(void* argument)
{
    (void)argument;
    __ada_raise_message(&__ada_exc_constraint_error, "child failure", 13);
}

int main(void)
{
    for (int iteration = 0; iteration < 20; ++iteration) {
        AdaTaskContext context = {0};
        AdaTaskContext* previous = __ada_task_context_bind(&context);
        AdaMaster master;
        void* objects[4];
        int elaborated = 1;
        atomic_store(&completed, 0);
        __ada_master_enter(&context, &master, ADA_MASTER_BLOCK);
        CHECK(!__ada_task_context_dispose(&context));
        for (int i = 0; i < 4; ++i) {
            __ada_task_create(&context, &objects[i], i == 2 ? failedTask : successfulTask,
                              NULL, &elaborated, NULL, NULL);
        }
        __ada_master_activate(&context, &master);
        CHECK(context.exception == &__ada_exc_tasking_error);
        __ada_raise_message(&__ada_exc_constraint_error, "parent failure", 14);
        __ada_master_await(&context, &master);
        CHECK(context.exception == &__ada_exc_constraint_error);
        CHECK(context.pendingMessageLength == 14 && memcmp(context.pendingMessage, "parent failure", 14) == 0);
        CHECK(atomic_load(&completed) == 3);
        __ada_master_leave(&context, &master);
        CHECK(__ada_task_context_dispose(&context));
        __ada_task_context_bind(previous);
    }
    // A group containing an unelaborated body starts no tasks at all.
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    AdaMaster master;
    void* objects[2];
    int elaborated = 1;
    int unavailable = 0;
    atomic_store(&completed, 0);
    __ada_master_enter(&context, &master, ADA_MASTER_BLOCK);
    __ada_task_create(&context, &objects[0], successfulTask, NULL, &unavailable, NULL, NULL);
    __ada_task_create(&context, &objects[1], successfulTask, NULL, &elaborated, NULL, NULL);
    __ada_master_activate(&context, &master);
    CHECK(context.exception == &__ada_exc_program_error);
    __ada_master_await(&context, &master);
    __ada_master_leave(&context, &master);
    CHECK(atomic_load(&completed) == 0);
    CHECK(__ada_task_context_dispose(&context));
    // Allocation in a shorter-lived scope still depends on the collection's
    // original lexical master. The outer await precedes collection cleanup.
    AdaMaster collection;
    AdaMaster inner;
    __ada_master_enter(&context, &master, ADA_MASTER_BLOCK);
    __ada_master_enter(&context, &collection, ADA_MASTER_COLLECTION);
    __ada_master_enter(&context, &inner, ADA_MASTER_SUBPROGRAM);
    __ada_task_create(&context, &objects[0], successfulTask, NULL, &elaborated, &collection, NULL);
    __ada_master_activate(&context, &collection);
    __ada_master_await(&context, &inner);
    __ada_master_leave(&context, &inner);
    __ada_master_await(&context, &master);
    CHECK(atomic_load(&completed) == 1);
    __ada_master_leave(&context, &collection);
    __ada_master_leave(&context, &master);
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    puts("task runtime passed");
    return 0;
}
