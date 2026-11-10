#include "../runtime/adart.h"
#include "../runtime/adathread.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "protected runtime check failed at line %d\n", __LINE__); abort(); \
} } while (0)

typedef struct Counter
{
    void* lock;
    int value;
} Counter;

typedef struct Worker
{
    Counter* object;
    atomic_int* arrived;
} Worker;

static void* update(void* argument)
{
    Worker* worker = argument;
    AdaTaskContext context = {0};
    __ada_task_context_bind(&context);
    for (int i = 0; i < 4000; ++i) {
        AdaProtectedAction action;
        __ada_protected_enter(&context, &worker->object->lock, &action);
        CHECK(context.exception == NULL && context.protectedActions == &action);
        CHECK(context.abortDeferralDepth == 1);
        ++worker->object->value;
        __ada_protected_leave(&context, &action);
        CHECK(context.abortDeferralDepth == 0);
    }
    CHECK(__ada_task_context_dispose(&context));
    return NULL;
}

static void* independent(void* argument)
{
    Worker* worker = argument;
    AdaTaskContext context = {0};
    __ada_task_context_bind(&context);
    AdaProtectedAction action;
    __ada_protected_enter(&context, &worker->object->lock, &action);
    atomic_fetch_add(worker->arrived, 1);
    int64_t start;
    CHECK(__ada_monotonic_clock(&start) == 0);
    while (atomic_load(worker->arrived) != 2) {
        int64_t now;
        CHECK(__ada_monotonic_clock(&now) == 0 && now - start < INT64_C(5000000000));
    }
    __ada_protected_leave(&context, &action);
    CHECK(__ada_task_context_dispose(&context));
    return NULL;
}

typedef struct FinalizationProbe
{
    Counter object;
    atomic_int entered;
    atomic_int destroying;
    atomic_int finished;
    int destroyed;
} FinalizationProbe;

static void* useProtected(void* argument)
{
    FinalizationProbe* probe = argument;
    AdaTaskContext context = {0};
    __ada_task_context_bind(&context);
    AdaProtectedAction action;
    __ada_protected_enter(&context, &probe->object.lock, &action);
    atomic_store(&probe->entered, 1);
    int64_t start;
    CHECK(__ada_monotonic_clock(&start) == 0);
    while (!atomic_load(&probe->destroying)) {
        int64_t now;
        CHECK(__ada_monotonic_clock(&now) == 0 && now - start < INT64_C(5000000000));
    }
    CHECK(!probe->destroyed);
    atomic_store(&probe->finished, 1);
    __ada_protected_leave(&context, &action);
    CHECK(__ada_task_context_dispose(&context));
    return NULL;
}

static void destroyProtected(void* argument)
{
    FinalizationProbe* probe = argument;
    atomic_store(&probe->destroying, 1);
    __ada_protected_destroy(&probe->object.lock);
    CHECK(atomic_load(&probe->finished));
    probe->destroyed = 1;
}

static void finalizeCoextension(void* argument)
{
    FinalizationProbe* probe = (FinalizationProbe*)((char*)argument - offsetof(FinalizationProbe, destroyed));
    CHECK(probe->destroyed && atomic_load(&probe->finished));
}

static void finalizationOrder(void)
{
    for (unsigned round = 0; round < 16; ++round) {
        FinalizationProbe probe = {0};
        atomic_init(&probe.entered, 0);
        atomic_init(&probe.destroying, 0);
        atomic_init(&probe.finished, 0);
        __ada_protected_init(&probe.object.lock);
        AdaFinalization* owner = NULL;
        AdaFinalization object, child;
        __ada_finalization_push(&owner, &object, &probe, destroyProtected);
        // A defaulted access discriminant creates its coextension after the
        // parent's partial-construction cleanup record has been reserved.
        __ada_finalization_push(&owner, &child, &probe.destroyed, finalizeCoextension);
        __ada_finalization_promote(&owner, &probe);
        AdaThread thread;
        CHECK(__ada_thread_create(&thread, useProtected, &probe) == 0);
        while (!atomic_load(&probe.entered)) { __ada_monotonic_delay_until(0); }
        __ada_finalize_to(&owner, NULL);
        CHECK(owner == NULL && probe.object.lock == NULL);
        CHECK(__ada_thread_join(thread, NULL) == 0);
    }
}

int main(void)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    Counter first = {0};
    Counter second = {0};
    __ada_protected_init(&first.lock);
    __ada_protected_init(&second.lock);
    CHECK(context.exception == NULL && first.lock != second.lock);
    AdaThread threads[4];
    Worker workers[4];
    for (int i = 0; i < 4; ++i) {
        workers[i] = (Worker) {&first, NULL};
        CHECK(__ada_thread_create(&threads[i], update, &workers[i]) == 0);
    }
    for (int i = 0; i < 4; ++i) {
        CHECK(__ada_thread_join(threads[i], NULL) == 0);
    }
    CHECK(first.value == 16000);
    atomic_int arrived = 0;
    for (int i = 0; i < 2; ++i) {
        workers[i] = (Worker) {i == 0 ? &first : &second, &arrived};
        CHECK(__ada_thread_create(&threads[i], independent, &workers[i]) == 0);
    }
    for (int i = 0; i < 2; ++i) {
        CHECK(__ada_thread_join(threads[i], NULL) == 0);
    }
    finalizationOrder();
    AdaProtectedAction outer;
    AdaProtectedAction inner;
    __ada_protected_enter(&context, &first.lock, &outer);
    CHECK(!__ada_task_context_dispose(&context));
    __ada_protected_enter(&context, &first.lock, &inner);
    CHECK(context.exception == &__ada_exc_program_error && context.protectedActions == &outer);
    __ada_raise_message(&__ada_exc_constraint_error, "preserved", 9);
    CHECK(context.abortDeferralDepth == 1);
    __ada_protected_enter(&context, &second.lock, &inner);
    CHECK(context.abortDeferralDepth == 2);
    __ada_protected_leave(&context, &inner);
    __ada_protected_leave(&context, &outer);
    CHECK(context.abortDeferralDepth == 0);
    CHECK(context.exception == &__ada_exc_constraint_error && context.pendingMessageLength == 9);
    CHECK(memcmp(context.pendingMessage, "preserved", 9) == 0 && context.protectedActions == NULL);
    __ada_protected_destroy(&first.lock);
    __ada_protected_destroy(&second.lock);
    CHECK(first.lock == NULL && second.lock == NULL);
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    puts("protected runtime passed");
    return 0;
}
