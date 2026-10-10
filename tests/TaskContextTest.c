#include "../runtime/adart.h"

#include "../runtime/adathread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "task context check failed at line %d\n", __LINE__); exit(1); \
} } while (0)

enum { workerCount = 4, iterations = 200 };

/* A reusable barrier using only the primitives available on all supported
   pthread hosts (macOS does not provide pthread_barrier_t). */
typedef struct Rendezvous
{
    AdaMutex m_mutex;
    AdaCondition m_condition;
    int m_arrived;
    int m_generation;
} Rendezvous;

static Rendezvous rendezvous;

static void meet(void)
{
    __ada_mutex_lock(&rendezvous.m_mutex);
    int generation = rendezvous.m_generation;
    if (++rendezvous.m_arrived == workerCount) {
        rendezvous.m_arrived = 0;
        ++rendezvous.m_generation;
        CHECK(__ada_condition_broadcast(&rendezvous.m_condition) == 0);
    } else {
        while (generation == rendezvous.m_generation) {
            CHECK(__ada_condition_wait(&rendezvous.m_condition, &rendezvous.m_mutex) == 0);
        }
    }
    __ada_mutex_unlock(&rendezvous.m_mutex);
}

typedef struct Object
{
    AdaTaskContext* m_context;
    int* m_count;
    int m_fail;
} Object;

static void finalizeObject(void* address)
{
    Object* object = address;
    CHECK(__ada_task_context() == object->m_context);
    CHECK(__ada_exception == NULL);
    ++*object->m_count;
    if (object->m_fail) {
        __ada_raise_message(ADA_CONSTRAINT_ERROR, "finalizer", 9);
    }
}

static void checkOccurrence(const AdaExceptionOccurrence* occurrence,
                            const AdaException* identity, const char* message, const char* location)
{
    CHECK(occurrence->identity == identity);
    CHECK(occurrence->length == (int)strlen(message));
    CHECK(memcmp(occurrence->message, message, strlen(message)) == 0);
    CHECK(occurrence->traceCount == 1);
    CHECK(strcmp(occurrence->origin, location) == 0);
    CHECK(strcmp(occurrence->trace[0].location, location) == 0);
}

static void testBindings(void)
{
    AdaTaskContext first = { 0 }, second = { 0 };
    CHECK(__ada_task_context() == &__ada_main_context);
    CHECK(__ada_task_context_bind(&first) == &__ada_main_context);
    AdaTraceFrame frame;
    __ada_trace_enter(&frame, "first", "first:1");
    __ada_raise_message(ADA_STORAGE_ERROR, "first", 5);
    CHECK(!__ada_task_context_dispose(&first));
    CHECK(first.m_exception == ADA_STORAGE_ERROR);

    int count = 0;
    Object object = { &first, &count, 0 };
    AdaFinalization* firstOwner = NULL;
    AdaFinalization* secondOwner = NULL;
    AdaFinalization firstRecord, secondRecord;
    __ada_finalization_push(&firstOwner, &firstRecord, &object, finalizeObject);
    CHECK(__ada_task_context_bind(&second) == &first);
    CHECK(__ada_exception == NULL && second.m_currentTrace == NULL);
    /* The same object address must find the active task's record, even when
       another context also has a registration for that address. */
    __ada_finalization_push(&secondOwner, &secondRecord, &object, finalizeObject);
    object.m_context = &second;
    CHECK(__ada_controlled_finalize(&object) == 0 && count == 1);
    CHECK(firstRecord.m_active && !secondRecord.m_active);
    CHECK(!__ada_task_context_dispose(&second));
    __ada_finalize_to(&secondOwner, NULL);
    __ada_raise_message(ADA_PROGRAM_ERROR, "second", 6);
    CHECK(__ada_task_context_bind(&first) == &second);
    CHECK(first.m_currentTrace == &frame && __ada_exception == ADA_STORAGE_ERROR);
    object.m_context = &first;
    __ada_finalize_to(&firstOwner, NULL);
    CHECK(count == 2 && __ada_exception == ADA_STORAGE_ERROR);
    AdaExceptionOccurrence occurrence;
    void* arena = NULL;
    __ada_exception_capture(&occurrence, &arena);
    checkOccurrence(&occurrence, ADA_STORAGE_ERROR, "first", "first:1");
    __ada_array_release(&arena);
    __ada_trace_leave(&frame);
    CHECK(__ada_task_context_bind(NULL) == &first);
    CHECK(__ada_task_context() == &__ada_main_context);
    CHECK(__ada_task_context_dispose(&first));
    CHECK(__ada_task_context_dispose(&second));
    CHECK(second.m_pendingMessage == NULL && second.m_exception == NULL);
    CHECK(__ada_task_context_dispose(&second));
    CHECK(!__ada_task_context_dispose(NULL));
}

static void* runWorker(void* argument)
{
    const char* name = argument;
    AdaException exception = { name };
    AdaTaskContext context = { 0 };
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    CHECK(previous == &__ada_main_context);
    for (int i = 0; i < iterations; ++i) {
        AdaTraceFrame frame;
        __ada_trace_enter(&frame, name, "initial");
        __ada_trace_location(name);
        int count = 0;
        AdaFinalization* owner = NULL;
        void* arena = NULL;
        void* collection = __ada_collection_create(&owner, &arena);
        CHECK(collection != NULL);
        Object* object = __ada_collection_allocate(collection, sizeof *object);
        CHECK(object != NULL);
        *object = (Object) { &context, &count, i % 2 };
        __ada_allocation_reserve(object, object, finalizeObject);
        __ada_controlled_activate(object, finalizeObject);
        CHECK(__ada_exception == NULL);
        __ada_raise_message(&exception, name, (int)strlen(name));
        meet();
        CHECK(__ada_task_context() == &context && context.m_currentTrace == &frame);
        AdaExceptionOccurrence occurrence;
        __ada_exception_capture(&occurrence, &arena);
        checkOccurrence(&occurrence, &exception, name, name);
        __ada_reraise(&occurrence);
        /* Alternate explicit deallocation and collection cleanup. Finalizers
           execute with no pending exception; successful cleanup restores it. */
        if (i % 3 == 0) {
            __ada_deallocate(object);
        }
        __ada_finalize_to(&owner, NULL);
        CHECK(count == 1 && owner == NULL);
        CHECK(context.m_registeredAllocations == NULL && context.m_registeredFinalizations == NULL);
        CHECK(__ada_exception == (i % 2 ? ADA_PROGRAM_ERROR : &exception));
        __ada_exception_capture(&occurrence, &arena);
        if (i % 2 == 0) {
            checkOccurrence(&occurrence, &exception, name, name);
        }
        __ada_array_release(&arena);
        __ada_trace_leave(&frame);
        CHECK(context.m_currentTrace == NULL);
        meet();
    }
    /* Disposing a detached context must release an uncaught pending message. */
    __ada_raise_message(&exception, name, (int)strlen(name));
    CHECK(__ada_task_context_bind(previous) == &context);
    CHECK(__ada_task_context_dispose(&context));
    return NULL;
}

int main(void)
{
    CHECK(__ada_mutex_init(&rendezvous.m_mutex) == 0);
    CHECK(__ada_condition_init(&rendezvous.m_condition) == 0);
    testBindings();
    __ada_raise_message(ADA_TASKING_ERROR, "main", 4);
    AdaThread workers[workerCount];
    char* names[workerCount] = { "one", "two", "three", "four" };
    for (int i = 0; i < workerCount; ++i) {
        CHECK(__ada_thread_create(&workers[i], runWorker, names[i]) == 0);
    }
    for (int i = 0; i < workerCount; ++i) {
        CHECK(__ada_thread_join(workers[i], NULL) == 0);
    }
    CHECK(__ada_exception == ADA_TASKING_ERROR);
    CHECK(__ada_main_context.m_pendingMessageLength == 4);
    CHECK(memcmp(__ada_main_context.m_pendingMessage, "main", 4) == 0);
    CHECK(__ada_task_context_dispose(&__ada_main_context));
    CHECK(__ada_condition_destroy(&rendezvous.m_condition) == 0);
    CHECK(__ada_mutex_destroy(&rendezvous.m_mutex) == 0);
    return 0;
}
