#include "../runtime/adart.h"
#include "../runtime/adathread.h"
#include "../runtime/adatime.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "select check failed at line %d\n", __LINE__); abort(); \
} } while (0)

enum { kRounds = 64 };

struct Server
{
    atomic_int release;
    atomic_int accepted;
    atomic_int finish;
};

struct Caller
{
    void* task;
    int selected;
    int value;
    int64_t deadline;
    atomic_int done;
};

static void yield(void)
{
    __ada_monotonic_delay_until(0);
}

static void server(void* argument)
{
    struct Server* data = argument;
    AdaTaskContext* context = __ada_task_context();
    __ada_task_activated(context);
    while (!atomic_load(&data->release)) { yield(); }
    AdaSelectChoice choices[] = {
        {ADA_SELECT_ACCEPT, 0, 7, 0, ADA_SELECT_MONOTONIC, 1},
        {ADA_SELECT_DELAY, 0, 0, 0, ADA_SELECT_MONOTONIC, 1}
    };
    void* request;
    // A queued call takes priority over an already expired delay.
    CHECK(__ada_task_select(context, choices, 2, 0, &request) == 0);
    int* value = __ada_task_entry_arguments(request);
    atomic_store(&data->accepted, 1);
    while (!atomic_load(&data->finish)) { yield(); }
    *value = 42;
    __ada_task_entry_complete(context, request);
    choices[1].kind = ADA_SELECT_TERMINATE;
    CHECK(__ada_task_select(context, choices, 2, 0, &request) == 1);
    CHECK(context->exception == NULL);
}

static void* timedCaller(void* argument)
{
    struct Caller* call = argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    call->deadline = __ada_real_clock() + INT64_C(1000000000);
    call->selected = __ada_task_entry_call_until(&context, call->task, 0, 7, &call->value,
        call->deadline, ADA_SELECT_MONOTONIC, ADA_ENTRY_TIMED);
    atomic_store(&call->done, 1);
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
        AdaMaster master;
        int elaborated = 1;
        void* object;
        struct Server data;
        atomic_init(&data.release, 0);
        atomic_init(&data.accepted, 0);
        atomic_init(&data.finish, 0);
        __ada_master_enter(&context, &master, ADA_MASTER_BLOCK);
        __ada_task_create(&context, &object, server, &data, &elaborated, NULL, NULL);
        __ada_master_activate(&context, &master);
        int value = -1;
        CHECK(!__ada_task_entry_call_until(&context, object, 0, 7, &value,
            0, ADA_SELECT_MONOTONIC, ADA_ENTRY_CONDITIONAL));
        CHECK(!__ada_task_entry_call_until(&context, object, 0, 7, &value,
            0, ADA_SELECT_MONOTONIC, ADA_ENTRY_TIMED));
        CHECK(__ada_task_entry_count(object, 0, 7) == 0 && value == -1);
        void* operation = __ada_async_start_task(&context, object, 0, 7, &value);
        CHECK(operation != NULL && __ada_async_run(operation));
        CHECK(__ada_task_entry_count(object, 0, 7) == 1);
        CHECK(!__ada_async_leave(&context, operation));
        CHECK(__ada_task_entry_count(object, 0, 7) == 0 && value == -1);
        if (round % 2 == 0) {
            // Once committed, even ending the part cannot cancel the call.
            operation = __ada_async_start_task(&context, object, 0, 7, &value);
            CHECK(__ada_async_run(operation));
            atomic_store(&data.release, 1);
            while (!atomic_load(&data.accepted)) { yield(); }
            atomic_store(&data.finish, 1);
            CHECK(__ada_async_leave(&context, operation));
            CHECK(value == 42);
        } else {
            struct Caller call = {.task = object, .value = -1};
            atomic_init(&call.done, 0);
            AdaThread thread;
            CHECK(__ada_thread_create(&thread, timedCaller, &call) == 0);
            while (__ada_task_entry_count(object, 0, 7) == 0) { yield(); }
            atomic_store(&data.release, 1);
            while (!atomic_load(&data.accepted)) { yield(); }
            if (round == 1) {
                __ada_monotonic_delay_until(call.deadline + INT64_C(1000000));
                CHECK(!atomic_load(&call.done));
            }
            atomic_store(&data.finish, 1);
            CHECK(__ada_thread_join(thread, NULL) == 0);
            CHECK(call.selected && call.value == 42);
        }
        __ada_master_await(&context, &master);
        __ada_master_leave(&context, &master);
        CHECK(context.exception == NULL && context.asyncSelects == NULL);
    }
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return 0;
}
