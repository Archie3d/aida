#include "../runtime/adart.h"
#include "../runtime/adathread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "abort deferral check failed at line %d\n", __LINE__); abort(); \
} } while (0)

typedef struct Object
{
    unsigned int depth;
    int count;
    int fail;
    int nested;
} Object;

static void finalize(void* address)
{
    Object* object = address;
    AdaTaskContext* context = __ada_task_context();
    CHECK(context->abortDeferralDepth == object->depth);
    CHECK(context->exception == NULL);
    ++object->count;
    if (object->nested) {
        AdaFinalization* owner = NULL;
        AdaFinalization record;
        Object inner = {object->depth + 1, 0, 0, 0};
        __ada_finalization_push(&owner, &record, &inner, finalize);
        __ada_finalize_to(&owner, NULL);
        CHECK(inner.count == 1 && context->abortDeferralDepth == object->depth);
    }
    if (object->fail) {
        __ada_raise_message(ADA_CONSTRAINT_ERROR, "finalizer", 9);
    }
}

static void finalizeView(void* address, void* tag)
{
    CHECK(tag == address);
    finalize(address);
}

static void* exercise(void* unused)
{
    (void)unused;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    for (int iteration = 0; iteration < 200; ++iteration) {
        __ada_abort_defer(&context);
        CHECK(!__ada_task_context_dispose(&context));
        AdaFinalization* owner = NULL;
        AdaFinalization records[2];
        Object first = {2, 0, 0, 1};
        Object second = {2, 0, iteration % 2, 0};
        __ada_finalization_push(&owner, &records[0], &first, finalize);
        __ada_finalization_push(&owner, &records[1], &second, finalize);
        __ada_raise_message(ADA_STORAGE_ERROR, "original", 8);
        __ada_finalize_to(&owner, NULL);
        CHECK(first.count == 1 && second.count == 1 && owner == NULL);
        CHECK(context.abortDeferralDepth == 1);
        if (second.fail) {
            CHECK(context.exception == ADA_PROGRAM_ERROR);
        } else {
            CHECK(context.exception == ADA_STORAGE_ERROR && context.pendingMessageLength == 8);
            CHECK(memcmp(context.pendingMessage, "original", 8) == 0);
        }
        __ada_finalize_to(&owner, NULL);
        CHECK(context.abortDeferralDepth == 1);
        __ada_abort_undefer(&context);
        CHECK(context.abortDeferralDepth == 0);
        CHECK(__ada_task_context_dispose(&context));

        Object view = {1, 0, iteration % 2, 0};
        __ada_finalization_push(&owner, &records[0], &view, finalize);
        CHECK(__ada_controlled_finalize_view(&view, finalizeView, &view) == view.fail);
        CHECK(view.count == 1 && context.abortDeferralDepth == 0);
        __ada_finalize_to(&owner, NULL);
        CHECK(view.count == 1 && context.exception == NULL);
        CHECK(__ada_task_context_dispose(&context));
    }
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    AdaThread workers[4];
    for (int i = 0; i < 4; ++i) {
        CHECK(__ada_thread_create(&workers[i], exercise, NULL) == 0);
    }
    for (int i = 0; i < 4; ++i) {
        CHECK(__ada_thread_join(workers[i], NULL) == 0);
    }
    puts("abort deferral passed");
    return 0;
}
