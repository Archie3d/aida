#include "../runtime/adart.h"
#include "../runtime/adathread.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "task entry check failed at line %d\n", __LINE__); abort(); \
} } while (0)

enum { kCallCount = 12, kMessageLength = 4097 };

struct CallData
{
    void* task;
    int value;
    int result;
    const AdaException* exception;
};

struct ServerData
{
    atomic_int release;
    char message[kMessageLength];
};

static void clearException(AdaTaskContext* context)
{
    context->exception = NULL;
    free(context->pendingMessage);
    context->pendingMessage = NULL;
    context->pendingMessageLength = 0;
}

static void* caller(void* argument)
{
    struct CallData* call = argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    __ada_task_entry_call(&context, call->task, 0, 0, call);
    call->exception = context.exception;
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

static void server(void* argument)
{
    struct ServerData* data = argument;
    AdaTaskContext* context = __ada_task_context();
    __ada_task_activated(context);
    while (!atomic_load(&data->release)) { __ada_monotonic_delay_until(0); }
    for (int i = 0; i < kCallCount; ++i) {
        void* request = __ada_task_entry_accept(context, 0, 0);
        struct CallData* call = __ada_task_entry_arguments(request);
        CHECK(call->value == i);
        CHECK(__ada_task_entry_caller(context) != NULL);
        call->result = i + 100;
        __ada_task_entry_complete(context, request);
    }
    void* request = __ada_task_entry_accept(context, 1, INT64_MAX);
    __ada_raise_message(ADA_CONSTRAINT_ERROR, data->message, kMessageLength);
    __ada_task_entry_complete(context, request);
    CHECK(context->exception == ADA_CONSTRAINT_ERROR);
    CHECK(context->pendingMessageLength == kMessageLength);
    clearException(context);
}

int main(void)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    AdaMaster master;
    int elaborated = 1;
    AdaConcurrentObject object;
    struct ServerData data;
    atomic_init(&data.release, 0);
    memset(data.message, 'x', sizeof data.message);
    struct CallData calls[kCallCount];
    AdaThread callers[kCallCount];
    __ada_master_enter(&context, &master, ADA_MASTER_BLOCK);
    __ada_task_create(&context, &object, server, &data, &elaborated, NULL, NULL);
    __ada_master_activate(&context, &master);
    for (int i = 0; i < kCallCount; ++i) {
        calls[i] = (struct CallData){ .task = object.resource, .value = i };
        CHECK(__ada_thread_create(&callers[i], caller, &calls[i]) == 0);
        while (__ada_task_entry_count(object.resource, 0, 0) != i + 1) { __ada_monotonic_delay_until(0); }
    }
    atomic_store(&data.release, 1);
    for (int i = 0; i < kCallCount; ++i) {
        __ada_thread_join(callers[i], NULL);
        CHECK(calls[i].result == i + 100 && calls[i].exception == NULL);
    }
    __ada_task_entry_call(&context, object.resource, 1, INT64_MAX, NULL);
    CHECK(context.exception == ADA_CONSTRAINT_ERROR && context.pendingMessageLength == kMessageLength);
    CHECK(memcmp(context.pendingMessage, data.message, kMessageLength) == 0);
    clearException(&context);
    __ada_master_await(&context, &master);
    __ada_task_entry_call(&context, object.resource, 0, 0, NULL);
    CHECK(context.exception == ADA_TASKING_ERROR);
    clearException(&context);
    __ada_master_leave(&context, &master);

    // Failure before activation must also wake callers already queued on the
    // object.resource, without starting a task body whose declarations were abandoned.
    __ada_master_enter(&context, &master, ADA_MASTER_BLOCK);
    __ada_task_create(&context, &object, server, &data, &elaborated, NULL, NULL);
    struct CallData abandoned = { .task = object.resource };
    AdaThread blocked;
    CHECK(__ada_thread_create(&blocked, caller, &abandoned) == 0);
    while (__ada_task_entry_count(object.resource, 0, 0) != 1) { __ada_monotonic_delay_until(0); }
    __ada_task_discard(&context, &master, &object, sizeof object);
    __ada_master_leave(&context, &master);
    __ada_thread_join(blocked, NULL);
    CHECK(abandoned.exception == ADA_TASKING_ERROR);
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    puts("task entry runtime OK");
    return 0;
}
