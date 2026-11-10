#include "../runtime/adart.h"
#include "../runtime/adataskservices.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "task services check failed at line %d\n", __LINE__); abort(); \
} } while (0)

enum { kWorkers = 4, kRounds = 500 };
static void* key;
static void* identity;
static void* collection;
static atomic_int finalized;
static void* createdByFinalizer;

typedef struct Value
{
    long first;
    long second;
    void* close;
    int fail;
} Value;

static void finalizeValue(void* address)
{
    Value* value = address;
    CHECK(__ada_exception == NULL);
    atomic_fetch_add(&finalized, 1);
    if (value->close != NULL) {
        __ada_task_attributes_close(value->close);
        createdByFinalizer = __ada_task_attributes_create();
    }
    if (value->fail) { __ada_raise_message(ADA_CONSTRAINT_ERROR, "finalizer", 9); }
}

static Value* newValue(void)
{
    Value* value = __ada_collection_allocate(collection, sizeof *value);
    CHECK(value != NULL);
    *value = (Value) {0};
    __ada_allocation_reserve(value, value, finalizeValue);
    __ada_controlled_activate(value, finalizeValue);
    return value;
}

static void* accessAttribute(void* argument)
{
    intptr_t lane = (intptr_t)argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    for (int i = 0; i < kRounds; ++i) {
        void* token = __ada_task_attribute_acquire(key, identity);
        CHECK(token != NULL);
        Value* value = __ada_task_attribute_get(token);
        CHECK(value->first == value->second);
        if (lane % 2 == 0) { value->first = i; value->second = i; }
        __ada_task_attribute_release(token);
        CHECK(context.exception == NULL && context.abortDeferralDepth == 0);
    }
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

typedef struct SuspensionJob
{
    void** slot;
    atomic_int entered;
    atomic_int returned;
} SuspensionJob;

static void* suspendTask(void* argument)
{
    SuspensionJob* job = argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    atomic_store(&job->entered, 1);
    __ada_suspension_wait(job->slot);
    CHECK(context.exception == NULL);
    atomic_store(&job->returned, 1);
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

static void testSuspensions(void)
{
    void* slot = NULL;
    CHECK(!__ada_suspension_state(&slot));
    __ada_suspension_set(&slot, 0);
    CHECK(slot == NULL);
    for (int i = 0; i < 100; ++i) {
        __ada_suspension_set(&slot, 1);
        CHECK(__ada_suspension_state(&slot));
        __ada_suspension_wait(&slot);
        CHECK(!__ada_suspension_state(&slot));
        SuspensionJob job = {.slot = &slot};
        AdaThread thread;
        CHECK(__ada_thread_create(&thread, suspendTask, &job) == 0);
        while (!atomic_load(&job.entered)) { __ada_monotonic_delay_until(0); }
        // Both publication orders are valid: stored permit or blocked waiter.
        __ada_suspension_set(&slot, 1);
        CHECK(__ada_thread_join(thread, NULL) == 0);
        CHECK(atomic_load(&job.returned) && !__ada_suspension_state(&slot));
    }
    __ada_suspension_destroy(&slot);
    CHECK(slot == NULL);
    __ada_suspension_destroy(&slot);
}

static void testAttributes(void)
{
    AdaFinalization* owner = NULL;
    void* arena = NULL;
    collection = __ada_collection_create(&owner, &arena);
    key = __ada_task_attributes_create();
    identity = __ada_current_task();
    void* token = __ada_task_attribute_acquire(key, identity);
    CHECK(token != NULL && __ada_task_attribute_get(token) == NULL);
    __ada_task_attribute_put(token, newValue());
    __ada_task_attribute_release(token);
    AdaThread workers[kWorkers];
    for (intptr_t i = 0; i < kWorkers; ++i) {
        CHECK(__ada_thread_create(&workers[i], accessAttribute, (void*)i) == 0);
    }
    for (int i = 0; i < kWorkers; ++i) { CHECK(__ada_thread_join(workers[i], NULL) == 0); }
    __ada_task_attributes_close(key);
    CHECK(atomic_load(&finalized) == 1);
    key = __ada_task_attributes_create();
    void* other = __ada_task_attributes_create();
    token = __ada_task_attribute_acquire(other, identity);
    Value* value = newValue();
    __ada_task_attribute_put(token, value);
    __ada_task_attribute_release(token);
    token = __ada_task_attribute_acquire(key, identity);
    value = newValue();
    value->close = other;
    __ada_task_attribute_put(token, value);
    __ada_task_attribute_release(token);
    __ada_raise_message(ADA_STORAGE_ERROR, "preserved", 9);
    __ada_task_attributes_terminated(identity);
    CHECK(atomic_load(&finalized) == 3 && createdByFinalizer != NULL);
    CHECK(__ada_exception == ADA_STORAGE_ERROR && __ada_task_context()->pendingMessageLength == 9);
    CHECK(memcmp(__ada_task_context()->pendingMessage, "preserved", 9) == 0);
    __ada_exception = NULL;
    __ada_task_attributes_close(key);
    __ada_task_attributes_close(createdByFinalizer);
    key = __ada_task_attributes_create();
    token = __ada_task_attribute_acquire(key, identity);
    value = newValue();
    value->fail = 1;
    __ada_task_attribute_put(token, value);
    __ada_task_attribute_release(token);
    __ada_raise_message(ADA_STORAGE_ERROR, "pending", 7);
    __ada_task_attributes_terminated(identity);
    CHECK(atomic_load(&finalized) == 4 && __ada_exception == ADA_PROGRAM_ERROR);
    __ada_exception = NULL;
    __ada_task_attributes_close(key);
    CHECK(__ada_task_attribute_acquire(NULL, NULL) == NULL && __ada_exception == ADA_PROGRAM_ERROR);
    __ada_exception = NULL;
    key = __ada_task_attributes_create();
    __ada_environment_task_state(1, 1);
    CHECK(__ada_task_attribute_acquire(key, identity) == NULL && __ada_exception == ADA_TASKING_ERROR);
    __ada_exception = NULL;
    __ada_environment_task_state(0, 0);
    __ada_task_attributes_close(key);
    __ada_finalize_to(&owner, NULL);
    __ada_array_release(&arena);
}

static void testStackAndPriority(void)
{
    int64_t baseline, rounded;
    CHECK(__ada_thread_stack_size(0, &baseline) == 0 && baseline > 0);
    CHECK(__ada_thread_stack_size(baseline + 1, &rounded) == 0 && rounded >= baseline + 1);
    CHECK(__ada_thread_stack_size(-1, &rounded) == EINVAL);
    CHECK(__ada_thread_stack_size(INT64_MAX, &rounded) != 0);
    CHECK(__ada_thread_set_priority(__ada_priority_thread_self(), -1) == EINVAL);
    CHECK(__ada_thread_set_priority(__ada_priority_thread_self(), kAdaAnyPriorityLast + 1) == EINVAL);
    CHECK(__ada_get_priority(__ada_current_task()) == kAdaDefaultPriority);
    __ada_set_priority(kAdaDefaultPriority, __ada_current_task());
    CHECK(__ada_exception == NULL);
    __ada_set_priority(kAdaAnyPriorityLast + 1, __ada_current_task());
    CHECK(__ada_exception == ADA_CONSTRAINT_ERROR);
    __ada_exception = NULL;
    CHECK(__ada_get_priority(NULL) == 0 && __ada_exception == ADA_PROGRAM_ERROR);
    __ada_exception = NULL;
    AdaConcurrentObject object = {0};
    __ada_protected_init(&object);
    __ada_protected_set_ceiling(&object, kAdaDefaultPriority - 1);
    AdaProtectedAction action;
    __ada_protected_enter(__ada_task_context(), &object, &action);
    CHECK(__ada_exception == ADA_PROGRAM_ERROR && __ada_task_context()->protectedActions == NULL);
    CHECK(__ada_task_context()->abortDeferralDepth == 0);
    __ada_exception = NULL;
    __ada_protected_destroy(&object);
}

int main(void)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    AdaMaster master;
    __ada_environment_task_state(0, 0);
    __ada_master_enter(&context, &master, ADA_MASTER_BLOCK);
    testStackAndPriority();
    testAttributes();
    testSuspensions();
    __ada_master_await(&context, &master);
    __ada_master_leave(&context, &master);
    CHECK(context.exception == NULL && __ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return 0;
}
