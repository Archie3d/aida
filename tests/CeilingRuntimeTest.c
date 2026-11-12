#include "../runtime/adart.h"
#include "../runtime/adathread.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "ceiling runtime check failed at line %d\n", __LINE__); abort(); \
} } while (0)

typedef struct Gate
{
    AdaConcurrentObject object;
    atomic_int arrived;
    int open;
    int count;
} Gate;

typedef struct Worker
{
    Gate* gate;
    int announced;
} Worker;

static int active(void)
{
    int base = __ada_get_priority(__ada_current_task());
    int inherited = atomic_load(&__ada_task_context()->inheritedPriority) - 1;
    return base > inherited ? base : inherited;
}

static int barrier(void* link, void* object)
{
    Worker* worker = link;
    Gate* gate = object;
    CHECK(active() == kAdaAnyPriorityLast);
    if (!gate->open && !worker->announced) {
        worker->announced = 1;
        atomic_fetch_add(&gate->arrived, 1);
    }
    return gate->open;
}

static void* waitForGate(void* argument)
{
    Worker* worker = argument;
    Gate* gate = worker->gate;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    AdaProtectedAction action;
    CHECK(__ada_protected_entry_enter_until(&context, &gate->object, &action,
        barrier, worker, 0, ADA_SELECT_MONOTONIC, ADA_ENTRY_WAIT));
    CHECK(context.exception == NULL && active() == kAdaAnyPriorityLast);
    ++gate->count;
    __ada_protected_complete(&context, &action);
    CHECK(context.exception == NULL && active() == kAdaDefaultPriority);
    CHECK(context.protectedActions == NULL && context.abortDeferralDepth == 0);
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    AdaPriorityThread self = __ada_priority_thread_self();
    int status = __ada_thread_set_priority(self, kAdaAnyPriorityLast);
    if (status == EPERM || status == EACCES || status == ENOTSUP) { return 77; }
    CHECK(status == 0 && __ada_thread_set_priority(self, kAdaDefaultPriority) == 0);
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    __ada_environment_task_state(0, 0);
    __ada_locking_policy(kAdaCeilingLocking);
    AdaConcurrentObject normal = {0};
    __ada_protected_init(&normal);
    AdaProtectedAction action;
    __ada_protected_enter(&context, &normal, &action);
    CHECK(context.exception == NULL && active() == kAdaPriorityLast);
    __ada_protected_leave(&context, &action);
    CHECK(context.exception == NULL && active() == kAdaDefaultPriority);
    __ada_protected_set_interrupt_ceiling(&normal, kAdaPriorityLast);
    CHECK(context.exception == ADA_CONSTRAINT_ERROR);
    context.exception = NULL;
    __ada_protected_set_ceiling(&normal, kAdaDefaultPriority - 1);
    __ada_protected_enter(&context, &normal, &action);
    CHECK(context.exception == ADA_PROGRAM_ERROR && context.protectedActions == NULL);
    context.exception = NULL;
    __ada_protected_destroy(&normal);
    for (int iteration = 0; iteration < 10; ++iteration) {
        Gate gate = {0};
        __ada_protected_init(&gate.object);
        __ada_protected_set_interrupt_ceiling(&gate.object, kAdaAnyPriorityLast);
        AdaThread workers[4];
        Worker arguments[4] = {{0}};
        for (int i = 0; i < 4; ++i) {
            arguments[i].gate = &gate;
            CHECK(__ada_thread_create(&workers[i], waitForGate, &arguments[i]) == 0);
        }
        while (atomic_load(&gate.arrived) < 4) { CHECK(__ada_monotonic_delay_until(0) == 0); }
        Worker cancelled = {&gate, 0};
        CHECK(!__ada_protected_entry_enter_until(&context, &gate.object, &action,
            barrier, &cancelled, 0, ADA_SELECT_MONOTONIC, ADA_ENTRY_CONDITIONAL));
        CHECK(context.exception == NULL && active() == kAdaDefaultPriority);
        CHECK(!__ada_protected_entry_enter_until(&context, &gate.object, &action,
            barrier, &cancelled, 0, ADA_SELECT_MONOTONIC, ADA_ENTRY_TIMED));
        CHECK(context.exception == NULL && context.protectedActions == NULL);
        __ada_protected_enter(&context, &gate.object, &action);
        CHECK(context.exception == NULL && active() == kAdaAnyPriorityLast);
        gate.open = 1;
        __ada_protected_complete(&context, &action);
        for (int i = 0; i < 4; ++i) { CHECK(__ada_thread_join(workers[i], NULL) == 0); }
        CHECK(gate.count == 4 && active() == kAdaDefaultPriority);
        __ada_protected_destroy(&gate.object);
    }
    __ada_locking_policy(kAdaNativeLocking);
    CHECK(context.exception == ADA_PROGRAM_ERROR);
    context.exception = NULL;
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return 0;
}
