#include "../runtime/adaentry.h"
#include "../runtime/adatime.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "requeue runtime check failed at line %d\n", __LINE__); abort(); \
} } while (0)

enum { kRounds = 64 };

struct Packet
{
    int* value;
};

struct Relay
{
    void* destination;
    int withAbort;
    atomic_int release;
};

static void destination(void* argument)
{
    struct Relay* relay = argument;
    AdaTaskContext* context = __ada_task_context();
    __ada_task_activated(context);
    while (!atomic_load(&relay->release)) { __ada_monotonic_delay_until(0); }
    AdaSelectChoice choices[] = {
        {ADA_SELECT_ACCEPT, 1, 7, 0, ADA_SELECT_MONOTONIC, 1},
        {ADA_SELECT_DELAY, 0, 0, 0, ADA_SELECT_MONOTONIC, 1}
    };
    void* request;
    int chosen = __ada_task_select(context, choices, 2, 0, &request);
    if (relay->withAbort) { CHECK(chosen == 1); }
    else {
        CHECK(chosen == 0);
        struct Packet* packet = __ada_task_entry_arguments(request);
        *packet->value += 2;
        __ada_task_entry_complete(context, request);
    }
}

static void source(void* argument)
{
    struct Relay* relay = argument;
    AdaTaskContext* context = __ada_task_context();
    __ada_task_activated(context);
    void* request = __ada_task_entry_accept(context, 0, 0);
    CHECK(context->currentEntry == request);
    struct Packet* packet = __ada_task_entry_arguments(request);
    *packet->value += 1;
    __ada_requeue_task(context, relay->destination, 1, 7, packet, sizeof *packet, relay->withAbort);
    CHECK(context->exception == NULL);
    __ada_task_entry_complete(context, request);
    CHECK(context->currentEntry == NULL);
}

int main(void)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    for (int round = 0; round < kRounds; ++round) {
        AdaMaster master, sourceMaster;
        AdaTaskContext callerContext = {0};
        int elaborated = 1, value = 0;
        void* from;
        struct Relay relay = {.withAbort = round % 2};
        atomic_init(&relay.release, 0);
        __ada_master_enter(&context, &master, ADA_MASTER_BLOCK);
        __ada_task_create(&context, &relay.destination, destination, &relay, &elaborated, NULL, NULL);
        __ada_master_activate(&context, &master);
        __ada_master_enter(&context, &sourceMaster, ADA_MASTER_BLOCK);
        __ada_task_create(&context, &from, source, &relay, &elaborated, NULL, NULL);
        __ada_master_activate(&context, &sourceMaster);
        struct Packet packet = {&value};
        // The independent caller and destination outlive the source's master.
        callerContext.currentMaster = &master;
        __ada_task_context_bind(&callerContext);
        void* operation = __ada_async_start_task(&callerContext, from, 0, 0, &packet);
        CHECK(operation != NULL && callerContext.exception == NULL);
        __ada_task_context_bind(&context);
        // Registration/cancellation follows the destination, not the source
        // task whose body may already have terminated after its requeue.
        while (__ada_task_entry_count(relay.destination, 1, 7) != 1) { __ada_monotonic_delay_until(0); }
        CHECK(value == 1);
        __ada_master_await(&context, &sourceMaster);
        __ada_master_leave(&context, &sourceMaster);
        __ada_task_context_bind(&callerContext);
        if (relay.withAbort) {
            CHECK(!__ada_async_leave(&callerContext, operation));
            CHECK(__ada_task_entry_count(relay.destination, 1, 7) == 0);
            atomic_store(&relay.release, 1);
        } else {
            atomic_store(&relay.release, 1);
            CHECK(__ada_async_leave(&callerContext, operation));
            CHECK(value == 3);
        }
        CHECK(callerContext.exception == NULL && callerContext.asyncSelects == NULL);
        callerContext.currentMaster = NULL;
        CHECK(__ada_task_context_dispose(&callerContext));
        __ada_task_context_bind(&context);
        __ada_master_await(&context, &master);
        __ada_master_leave(&context, &master);
    }
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return 0;
}
