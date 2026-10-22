#include "../runtime/adart.h"
#include "../runtime/adathread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "entry runtime check failed at line %d\n", __LINE__); abort(); \
} } while (0)

typedef struct Buffer
{
    void* lock;
    int full;
    int value;
    int sum;
    int broken;
} Buffer;

typedef struct Worker
{
    Buffer* buffer;
    int producer;
} Worker;

static int canPut(void* link, void* object)
{
    CHECK(__ada_task_context()->abortDeferralDepth > 0);
    CHECK(link == object);
    return !((Buffer*)object)->full;
}

static int canGet(void* link, void* object)
{
    CHECK(__ada_task_context()->abortDeferralDepth > 0);
    CHECK(link == object);
    return ((Buffer*)object)->full;
}

static int brokenBarrier(void* link, void* object)
{
    (void)link;
    if (((Buffer*)object)->broken) {
        __ada_raise_message(ADA_CONSTRAINT_ERROR, "barrier failed", 14);
    }
    return 0;
}

static int closedBarrier(void* link, void* object)
{
    (void)link;
    (void)object;
    return 0;
}

static void* exchange(void* argument)
{
    Worker* worker = argument;
    Buffer* buffer = worker->buffer;
    AdaTaskContext context = {0};
    __ada_task_context_bind(&context);
    for (int i = 1; i <= 1000; ++i) {
        AdaProtectedAction action;
        __ada_protected_entry_enter(&context, &buffer->lock, &action,
            worker->producer ? canPut : canGet, buffer);
        CHECK(context.exception == NULL && context.protectedActions == &action);
        CHECK(context.abortDeferralDepth == 1);
        if (worker->producer) {
            buffer->value = i;
            buffer->full = 1;
        } else {
            buffer->sum += buffer->value;
            buffer->full = 0;
        }
        __ada_protected_complete(&context, &action);
        CHECK(context.exception == NULL && context.protectedActions == NULL);
        CHECK(context.abortDeferralDepth == 0);
    }
    CHECK(__ada_task_context_dispose(&context));
    return NULL;
}

static void* failQueued(void* argument)
{
    Worker* worker = argument;
    Buffer* buffer = worker->buffer;
    AdaTaskContext context = {0};
    __ada_task_context_bind(&context);
    AdaProtectedAction action;
    __ada_protected_entry_enter(&context, &buffer->lock, &action,
        worker->producer ? brokenBarrier : closedBarrier, buffer);
    CHECK(context.exception == ADA_PROGRAM_ERROR && context.protectedActions == NULL);
    CHECK(context.abortDeferralDepth == 0);
    CHECK(__ada_task_context_dispose(&context));
    return NULL;
}

int main(void)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    Buffer buffer = {0};
    __ada_protected_init(&buffer.lock);
    CHECK(context.exception == NULL);
    AdaThread threads[4];
    Worker workers[4];
    for (int i = 0; i < 4; ++i) {
        workers[i] = (Worker) {&buffer, i < 2};
        CHECK(__ada_thread_create(&threads[i], exchange, &workers[i]) == 0);
    }
    for (int i = 0; i < 4; ++i) {
        CHECK(__ada_thread_join(threads[i], NULL) == 0);
    }
    CHECK(buffer.sum == 1001000 && buffer.full == 0);
    for (int i = 0; i < 2; ++i) {
        workers[i].producer = i == 0;
        CHECK(__ada_thread_create(&threads[i], failQueued, &workers[i]) == 0);
    }
    int count;
    do {
        AdaProtectedAction action;
        __ada_protected_enter(&context, &buffer.lock, &action);
        count = __ada_protected_count(&buffer.lock, brokenBarrier)
            + __ada_protected_count(&buffer.lock, closedBarrier);
        __ada_protected_leave(&context, &action);
    } while (count != 2);
    AdaProtectedAction action;
    __ada_protected_enter(&context, &buffer.lock, &action);
    buffer.broken = 1;
    __ada_raise_message(ADA_CONSTRAINT_ERROR, "opener preserved", 16);
    __ada_protected_complete(&context, &action);
    CHECK(context.exception == ADA_CONSTRAINT_ERROR);
    CHECK(context.pendingMessageLength == 16 && memcmp(context.pendingMessage, "opener preserved", 16) == 0);
    for (int i = 0; i < 2; ++i) {
        CHECK(__ada_thread_join(threads[i], NULL) == 0);
    }
    // Entry calls on any target from an action are diagnosed before waiting.
    context.exception = NULL;
    __ada_protected_enter(&context, &buffer.lock, &action);
    AdaProtectedAction nested;
    __ada_protected_entry_enter(&context, &buffer.lock, &nested, closedBarrier, NULL);
    CHECK(context.exception == ADA_PROGRAM_ERROR && context.protectedActions == &action);
    __ada_protected_leave(&context, &action);
    __ada_protected_destroy(&buffer.lock);
    CHECK(buffer.lock == NULL);
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return 0;
}
