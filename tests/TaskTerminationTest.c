#include "../runtime/adart.h"
#include "../runtime/adatermination.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "termination check failed at line %d\n", __LINE__); abort(); \
} } while (0)

enum { kJobs = 3, kMessageLength = 500 };
typedef struct Log
{
    AdaConcurrentObject object;
    int causes[3];
    int calls;
} Log;
typedef struct Job
{
    AdaConcurrentObject object;
    enum AdaTerminationCause cause;
} Job;
static char message[kMessageLength];
static atomic_int released;
static atomic_int stopChanging;
static atomic_int changes;
static AdaTerminationHandler first, second;

static void record(void* object, enum AdaTerminationCause cause, void* identity,
    const AdaExceptionOccurrence* occurrence)
{
    Log* log = object;
    CHECK(identity == __ada_current_task());
    CHECK(!__ada_task_is_terminated(identity) && !__ada_task_is_callable(identity));
    CHECK(__ada_task_context()->protectedActions != NULL && __ada_exception == NULL);
    CHECK(cause >= kTerminationNormal && cause <= kTerminationUnhandledException);
    if (cause == kTerminationUnhandledException) {
        CHECK(occurrence->identity == ADA_CONSTRAINT_ERROR && occurrence->length == kMessageLength);
        CHECK(memcmp(occurrence->message, message, kMessageLength) == 0);
    } else { CHECK(occurrence->identity == NULL && occurrence->length == 0); }
    // Selection must release the master lock before entering user code.
    __ada_specific_handler(identity);
    CHECK(__ada_exception == NULL);
    __ada_set_dependents_handler(NULL);
    ++log->causes[cause];
    ++log->calls;
    __ada_raise_message(ADA_STORAGE_ERROR, "ignored", 7);
}

static void recordLinked(void* link, void* object, enum AdaTerminationCause cause, void* identity,
    const AdaExceptionOccurrence* occurrence)
{
    CHECK(link == &second);
    record(object, cause, identity, occurrence);
}

static void taskBody(void* argument)
{
    Job* job = argument;
    AdaTaskContext* context = __ada_task_context();
    __ada_task_activated(context);
    while (!atomic_load(&released)) { __ada_async_poll(context); }
    if (job->cause == kTerminationAbnormal) {
        __ada_abort_task(__ada_current_task());
        __ada_async_poll(context);
    } else if (job->cause == kTerminationUnhandledException) {
        __ada_raise_message(ADA_CONSTRAINT_ERROR, message, kMessageLength);
    }
}

static void* changeHandler(void* argument)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    while (!atomic_load(&stopChanging)) {
        __ada_set_specific_handler(argument, &first);
        CHECK(__ada_specific_handler(argument) == &first);
        __ada_set_specific_handler(argument, &second);
        CHECK(__ada_specific_handler(argument) == &second);
        atomic_fetch_add(&changes, 1);
    }
    CHECK(context.exception == NULL && __ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    __ada_environment_task_state(0, 0);
    memset(message, 'x', sizeof message);
    Log fallback = {0}, specific = {0};
    __ada_protected_init(&fallback.object);
    __ada_protected_init(&specific.object);
    first = (AdaTerminationHandler) {(void (*)(void))record, NULL, &fallback.object};
    second = (AdaTerminationHandler) {(void (*)(void))recordLinked, &second, &specific.object};
    CHECK(__ada_current_fallback_handler() == NULL);
    __ada_set_dependents_handler(&first);
    CHECK(__ada_current_fallback_handler() == &first);
    AdaMaster master;
    __ada_master_enter(&context, &master, ADA_MASTER_BLOCK);
    int elaborated = 1;
    Job jobs[kJobs] = {0};
    for (int i = 0; i < kJobs; ++i) {
        jobs[i].cause = i == 0 ? kTerminationNormal : i == 1 ? kTerminationAbnormal : kTerminationUnhandledException;
        __ada_task_create(&context, &jobs[i].object, taskBody, &jobs[i], &elaborated, NULL, NULL);
    }
    __ada_master_activate(&context, &master);
    AdaThread changer;
    CHECK(__ada_thread_create(&changer, changeHandler, jobs[0].object.resource) == 0);
    // Concurrent Set/Get is legal while the task remains alive.
    while (atomic_load(&changes) < 1000) { __ada_specific_handler(jobs[0].object.resource); }
    atomic_store(&stopChanging, 1);
    CHECK(__ada_thread_join(changer, NULL) == 0);
    __ada_set_specific_handler(jobs[0].object.resource, &second);
    atomic_store(&released, 1);
    __ada_master_await(&context, &master);
    __ada_master_await(&context, &master);
    CHECK(context.exception == NULL && specific.calls == 1 && fallback.calls == 2);
    CHECK(specific.causes[kTerminationNormal] == 1);
    CHECK(fallback.causes[kTerminationAbnormal] == 1 && fallback.causes[kTerminationUnhandledException] == 1);
    CHECK(__ada_specific_handler(jobs[0].object.resource) == NULL && context.exception == ADA_TASKING_ERROR);
    context.exception = NULL;
    __ada_set_specific_handler(jobs[0].object.resource, NULL);
    CHECK(context.exception == ADA_TASKING_ERROR);
    context.exception = NULL;
    __ada_master_leave(&context, &master);
    CHECK(__ada_specific_handler(NULL) == NULL && context.exception == ADA_PROGRAM_ERROR);
    context.exception = NULL;
    __ada_set_specific_handler(NULL, &first);
    CHECK(context.exception == ADA_PROGRAM_ERROR);
    context.exception = NULL;
    // Unactivated tasks notify once in their own context, preserving the creator.
    __ada_master_enter(&context, &master, ADA_MASTER_BLOCK);
    Job unactivated = {0};
    __ada_task_create(&context, &unactivated.object, taskBody, &unactivated, &elaborated, NULL, NULL);
    __ada_raise_message(ADA_CONSTRAINT_ERROR, "creator", 7);
    __ada_master_await(&context, &master);
    CHECK(context.exception == ADA_CONSTRAINT_ERROR && context.pendingMessageLength == 7);
    CHECK(memcmp(context.pendingMessage, "creator", 7) == 0 && fallback.calls == 3);
    context.exception = NULL;
    __ada_master_leave(&context, &master);
    __ada_set_dependents_handler(NULL);
    __ada_protected_destroy(&fallback.object);
    __ada_protected_destroy(&specific.object);
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return 0;
}
