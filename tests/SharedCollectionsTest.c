#include "../runtime/adart.h"
#include "../runtime/adathread.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "shared collection check failed at line %d\n", __LINE__); abort(); \
} } while (0)

enum { workerCount = 4, objectCount = 200 };

typedef struct Shared Shared;
typedef struct Object
{
    Shared* shared;
    int reenter;
    int fail;
} Object;

typedef struct Job
{
    void* task;
    Shared* shared;
    int spawn;
} Job;

struct Shared
{
    void* collection;
    void* tasks;
    Object* objects[workerCount][objectCount];
    atomic_int finalized;
    atomic_int completed;
    atomic_int open;
};

typedef struct Worker
{
    Shared* shared;
    int index;
} Worker;

static void finalizeObject(void* address)
{
    Object* object = address;
    AdaTaskContext* context = __ada_task_context();
    CHECK(context->exception == NULL && context->abortDeferralDepth > 0);
    atomic_fetch_add(&object->shared->finalized, 1);
    if (object->reenter) {
        Object* nested = __ada_collection_allocate(object->shared->collection, sizeof *nested);
        CHECK(nested != NULL);
        *nested = (Object) {object->shared, 0, 0};
        __ada_allocation_reserve(nested, nested, finalizeObject);
        __ada_controlled_activate(nested, finalizeObject);
        __ada_deallocate(nested);
    }
    if (object->fail) {
        __ada_raise_message(ADA_CONSTRAINT_ERROR, "shared finalizer", 16);
    }
}

static void* produce(void* argument)
{
    Worker* worker = argument;
    AdaTaskContext context = {0};
    __ada_task_context_bind(&context);
    for (int i = 0; i < objectCount; ++i) {
        Object* object;
        if (i % 2 == 0) {
            object = __ada_collection_allocate(worker->shared->collection, sizeof *object);
            CHECK(object != NULL);
            __ada_allocation_reserve(object, object, finalizeObject);
        } else {
            // The limited/class-wide result path also transfers registration
            // and arena ownership independently of the allocating task.
            void* pending = __ada_collection_begin(worker->shared->collection);
            CHECK(pending != NULL);
            object = __ada_array_local(__ada_collection_result_arena(pending), 1, 1, sizeof *object);
            CHECK(object != NULL);
            __ada_finalization_reserve(__ada_collection_result_owner(pending),
                __ada_collection_result_arena(pending), object, finalizeObject);
            __ada_collection_finish(pending, object);
        }
        *object = (Object) {worker->shared, i % 20 == 0, i == 1};
        __ada_controlled_activate(object, finalizeObject);
        CHECK(context.exception == NULL);
        worker->shared->objects[worker->index][i] = object;
    }
    // Heap registrations do not pin an allocator's stack or task context.
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(NULL);
    return NULL;
}

static void* consume(void* argument)
{
    Worker* worker = argument;
    AdaTaskContext context = {0};
    __ada_task_context_bind(&context);
    for (int i = 0; i < objectCount; ++i) {
        __ada_raise_message(ADA_STORAGE_ERROR, "consumer", 8);
        __ada_deallocate(worker->shared->objects[worker->index][i]);
        CHECK(context.abortDeferralDepth == 0);
        if (i == 1) {
            CHECK(context.exception == ADA_PROGRAM_ERROR);
        } else {
            CHECK(context.exception == ADA_STORAGE_ERROR && context.pendingMessageLength == 8);
            CHECK(memcmp(context.pendingMessage, "consumer", 8) == 0);
        }
    }
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(NULL);
    return NULL;
}

static int elaborated = 1;
static void taskBody(void* argument)
{
    Job* job = argument;
    Shared* shared = job->shared;
    AdaTaskContext* context = __ada_task_context();
    __ada_task_activated(context);
    while (!atomic_load(&shared->open)) {
        __ada_monotonic_delay_until(0);
    }
    if (job->spawn) {
        Job* child = __ada_collection_allocate(shared->tasks, sizeof *child);
        CHECK(child != NULL);
        child->shared = shared;
        child->spawn = 0;
        __ada_task_create(context, &child->task, taskBody, child, &elaborated, shared->tasks, child);
        __ada_task_activate_group(context, shared->tasks, child);
        CHECK(context->exception == NULL);
    }
    atomic_fetch_add(&shared->completed, 1);
}

static void* spawn(void* argument)
{
    Worker* worker = argument;
    AdaTaskContext context = {0};
    __ada_task_context_bind(&context);
    Job* job = __ada_collection_allocate(worker->shared->tasks, sizeof *job);
    CHECK(job != NULL);
    job->shared = worker->shared;
    job->spawn = 1;
    __ada_task_create(&context, &job->task, taskBody, job, &elaborated, worker->shared->tasks, job);
    __ada_task_activate_group(&context, worker->shared->tasks, job);
    CHECK(context.exception == NULL);
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(NULL);
    return NULL;
}

static void runWorkers(Shared* shared, AdaThreadEntry entry)
{
    AdaThread threads[workerCount];
    Worker workers[workerCount];
    for (int i = 0; i < workerCount; ++i) {
        workers[i] = (Worker) {shared, i};
        CHECK(__ada_thread_create(&threads[i], entry, &workers[i]) == 0);
    }
    for (int i = 0; i < workerCount; ++i) {
        CHECK(__ada_thread_join(threads[i], NULL) == 0);
    }
}

int main(void)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    for (int iteration = 0; iteration < 10; ++iteration) {
        AdaMaster master;
        AdaFinalization* owner = NULL;
        void* arena = NULL;
        Shared shared = {0};
        __ada_master_enter(&context, &master, ADA_MASTER_BLOCK);
        shared.collection = __ada_collection_create(&owner, &arena);
        shared.tasks = __ada_collection_create(&owner, &arena);
        CHECK(shared.collection != NULL && shared.tasks != NULL);
        runWorkers(&shared, produce);
        CHECK(atomic_load(&shared.finalized) == 0);
        runWorkers(&shared, consume);
        CHECK(atomic_load(&shared.finalized) == workerCount * (objectCount + objectCount / 20));
        runWorkers(&shared, spawn);
        CHECK(atomic_load(&shared.completed) == 0);
        atomic_store(&shared.open, 1);
        __ada_master_await(&context, &master);
        CHECK(atomic_load(&shared.completed) == 2 * workerCount);
        __ada_finalize_to(&owner, NULL);
        __ada_array_release(&arena);
        __ada_master_leave(&context, &master);
        CHECK(__ada_task_context_dispose(&context));
    }
    __ada_task_context_bind(previous);
    CHECK(__ada_main_context.registeredAllocations == NULL);
    CHECK(__ada_main_context.sharedFinalizations == NULL);
    CHECK(__ada_main_context.sharedMasters == NULL);
    puts("shared collections passed");
    return 0;
}
