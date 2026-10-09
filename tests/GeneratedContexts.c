#include "../runtime/adart.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "generated context check failed at line %d\n", __LINE__); exit(1); \
} } while (0)

enum { workerCount = 4 };
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
static int arrived;
static int generation;

extern int context_worker__run(int mode);

void contextMeet(void)
{
    AdaTraceFrame* frame = __ada_task_context()->m_currentTrace;
    CHECK(frame != NULL && strcmp(frame->routine, "Fail") == 0);
    CHECK(strstr(frame->location, "context_worker.adb:") != NULL);
    frame = frame->previous;
    CHECK(frame != NULL && strcmp(frame->routine, "Invoke") == 0);
    frame = frame->previous;
    CHECK(frame != NULL && strcmp(frame->routine, "Run") == 0 && frame->previous == NULL);
    CHECK(pthread_mutex_lock(&mutex) == 0);
    int previous = generation;
    if (++arrived == workerCount) {
        arrived = 0;
        ++generation;
        CHECK(pthread_cond_broadcast(&condition) == 0);
    } else {
        while (previous == generation) {
            CHECK(pthread_cond_wait(&condition, &mutex) == 0);
        }
    }
    CHECK(pthread_mutex_unlock(&mutex) == 0);
}

static void* worker(void* argument)
{
    int mode = *(int*)argument;
    AdaTaskContext context = { 0 };
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    for (int i = 0; i < 40; ++i) {
        int result = context_worker__run(mode);
        CHECK(context.m_currentTrace == NULL);
        CHECK(context.m_registeredFinalizations == NULL && context.m_registeredAllocations == NULL);
        if (mode == 0) {
            CHECK(result == 17 && context.m_exception == NULL);
        } else {
            CHECK(context.m_exception == ADA_PROGRAM_ERROR);
            void* arena = NULL;
            AdaExceptionOccurrence occurrence;
            __ada_exception_capture(&occurrence, &arena);
            CHECK(occurrence.length == 15 && memcmp(occurrence.message, "uncaught worker", 15) == 0);
            CHECK(occurrence.traceCount == 1);
            CHECK(strcmp(occurrence.trace[0].routine, "Run") == 0);
            CHECK(strstr(occurrence.origin, "context_worker.adb:") != NULL);
            __ada_array_release(&arena);
        }
    }
    __ada_task_context_bind(previous);
    CHECK(__ada_task_context_dispose(&context));
    return NULL;
}

int exerciseContexts(void)
{
    AdaTaskContext* mainContext = __ada_task_context();
    AdaTraceFrame* mainTrace = mainContext->m_currentTrace;
    __ada_raise_message(ADA_TASKING_ERROR, "main sentinel", 13);
    pthread_t threads[workerCount];
    int modes[workerCount] = { 0, 1, 0, 1 };
    for (int i = 0; i < workerCount; ++i) {
        CHECK(pthread_create(&threads[i], NULL, worker, &modes[i]) == 0);
    }
    for (int i = 0; i < workerCount; ++i) {
        CHECK(pthread_join(threads[i], NULL) == 0);
    }
    CHECK(mainContext->m_exception == ADA_TASKING_ERROR && mainContext->m_currentTrace == mainTrace);
    void* arena = NULL;
    AdaExceptionOccurrence occurrence;
    __ada_exception_capture(&occurrence, &arena);
    CHECK(occurrence.length == 13 && memcmp(occurrence.message, "main sentinel", 13) == 0);
    __ada_array_release(&arena);
    return 0;
}
