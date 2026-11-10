#include "../runtime/adart.h"
#include "../runtime/adatime.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "abort runtime check failed at line %d\n", __LINE__); abort(); \
} } while (0)

enum AbortMode
{
    kDelay,
    kDeferred,
    kActivation,
    kBeforeStart
};

enum { kRounds = 32 };

struct Worker
{
    enum AbortMode mode;
    AdaConcurrentObject identity;
    atomic_int ready;
    atomic_int release;
    atomic_int observed;
};

static void body(void* argument)
{
    struct Worker* worker = argument;
    AdaTaskContext* context = __ada_task_context();
    CHECK(worker->mode != kBeforeStart);
    if (worker->mode != kActivation) { __ada_task_activated(context); }
    if (worker->mode == kDeferred) { __ada_abort_defer(context); }
    atomic_store(&worker->ready, 1);
    if (worker->mode == kDeferred) {
        while (!atomic_load(&worker->release)) {
            __ada_async_poll(context);
            CHECK(context->exception == NULL);
            __ada_monotonic_delay_until(0);
        }
        __ada_abort_undefer(context);
        __ada_async_poll(context);
    } else { __ada_delay_until(context, __ada_real_clock() + INT64_C(60000000000)); }
    CHECK(context->exception == &__ada_exc_async_transfer);
    atomic_store(&worker->observed, 1);
}

static void* abortActivation(void* argument)
{
    struct Worker* worker = argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    while (!atomic_load(&worker->ready)) { __ada_monotonic_delay_until(0); }
    __ada_abort_tasks(&context, &worker->identity.resource, 1);
    CHECK(context.exception == NULL);
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    for (int round = 0; round < kRounds; ++round) {
        for (enum AbortMode mode = kDelay; mode <= kBeforeStart; ++mode) {
            AdaMaster master;
            int elaborated = 1;
            struct Worker worker = {.mode = mode};
            atomic_init(&worker.ready, 0);
            atomic_init(&worker.release, 0);
            atomic_init(&worker.observed, 0);
            __ada_master_enter(&context, &master, ADA_MASTER_BLOCK);
            __ada_task_create(&context, &worker.identity, body, &worker, &elaborated, NULL, NULL);
            AdaThread aborter;
            if (mode == kActivation) { CHECK(__ada_thread_create(&aborter, abortActivation, &worker) == 0); }
            if (mode == kBeforeStart) { __ada_abort_tasks(&context, &worker.identity.resource, 1); }
            __ada_master_activate(&context, &master);
            CHECK(context.exception == NULL);
            if (mode == kActivation) { CHECK(__ada_thread_join(aborter, NULL) == 0); }
            else if (mode != kBeforeStart) {
                while (!atomic_load(&worker.ready)) { __ada_monotonic_delay_until(0); }
                __ada_abort_tasks(&context, &worker.identity.resource, 1);
                CHECK(!__ada_task_is_callable(worker.identity.resource));
                if (mode == kDeferred) {
                    CHECK(!atomic_load(&worker.observed));
                    atomic_store(&worker.release, 1);
                }
            }
            __ada_master_await(&context, &master);
            CHECK(__ada_task_is_terminated(worker.identity.resource));
            CHECK(atomic_load(&worker.observed) == (mode != kBeforeStart));
            __ada_abort_tasks(&context, &worker.identity.resource, 1);
            CHECK(context.exception == NULL);
            __ada_master_leave(&context, &master);
        }
    }
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return 0;
}
