#include "../runtime/adart.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "storage pools check failed at line %d\n", __LINE__); abort(); \
} } while (0)

enum { kWorkerCount = 4, kIterations = 300 };

typedef struct Header
{
    int64_t size;
    int64_t alignment;
} Header;

typedef struct Pool
{
    AdaTag* tag;
    void* reentrantCollection;
    atomic_int allocations;
    atomic_int releases;
    atomic_int queries;
    int mode;
    void* invalidBase;
    AdaFinalization** closeOwner;
} Pool;

static void allocate(void* link, void* object, void** address, int64_t size, int64_t alignment)
{
    Pool* pool = object;
    CHECK(link == pool && __ada_exception == NULL);
    if (pool->mode == 1) { __ada_raise_message(ADA_STORAGE_ERROR, "pool refused", 12); return; }
    if (pool->mode == 2) { *address = NULL; return; }
    void* nested = __ada_collection_allocate(pool->reentrantCollection, 8);
    CHECK(nested != NULL && __ada_exception == NULL);
    __ada_deallocate(nested);
    Header* header = malloc(sizeof *header + (size == 0 ? 1 : (size_t)size) + 1);
    CHECK(header != NULL);
    *header = (Header) {size, alignment};
    *address = header + 1;
    if (pool->mode == 3) {
        pool->invalidBase = header;
        *address = (char*)*address + 1;
    }
    atomic_fetch_add(&pool->allocations, 1);
    if (pool->mode == 5) {
        __ada_finalize_to(pool->closeOwner, NULL);
    }
}

static void deallocate(void* link, void* object, void* address, int64_t size, int64_t alignment)
{
    Pool* pool = object;
    AdaTaskContext* context = __ada_task_context();
    CHECK(link == pool && context->exception == NULL && context->abortDeferralDepth > 0);
    AdaTraceFrame trace;
    __ada_trace_enter(&trace, "pool callback", "pool release");
    Header* header = pool->mode == 3 ? pool->invalidBase : (Header*)address - 1;
    CHECK(header->size == size && header->alignment == alignment);
    // Calling back into registration/deallocation must not deadlock.
    void* nested = __ada_collection_allocate(pool->reentrantCollection, 8);
    CHECK(nested != NULL && context->exception == NULL);
    __ada_deallocate(nested);
    free(header);
    atomic_fetch_add(&pool->releases, 1);
    if (pool->mode == 4) { __ada_raise_message(ADA_CONSTRAINT_ERROR, "pool release", 12); }
    __ada_trace_leave_context(context, &trace);
}

static int64_t storageSize(void* link, void* object)
{
    Pool* pool = object;
    CHECK(link == pool);
    atomic_fetch_add(&pool->queries, 1);
    return INT64_C(1) << 40;
}

static void clearException(const AdaException* expected)
{
    AdaTaskContext* context = __ada_task_context();
    CHECK(context->exception == expected && context->abortDeferralDepth == 0);
    free(context->pendingMessage);
    context->pendingMessage = NULL;
    context->pendingMessageLength = 0;
    context->exception = NULL;
}

typedef struct Worker
{
    void* collection;
} Worker;

static void* exercise(void* argument)
{
    Worker* worker = argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    AdaTraceFrame trace;
    __ada_trace_enter(&trace, "allocator", "initialization");
    static const int64_t kAlignments[] = {0, 1, 2, 4, 8};
    for (int i = 0; i < kIterations; ++i) {
        int64_t size = i % 65;
        int64_t alignment = kAlignments[i % 5];
        unsigned char* object = __ada_collection_allocate_aligned(worker->collection, size, alignment);
        CHECK(object != NULL && context.exception == NULL);
        for (int64_t n = 0; n < size; ++n) { CHECK(object[n] == 0); object[n] = 93; }
        __ada_raise_message(ADA_CONSTRAINT_ERROR, "initializer", 11);
        const char* message = context.pendingMessage;
        const char* origin = context.pendingOrigin;
        int count = context.pendingTraceCount;
        __ada_deallocate(object);
        CHECK(context.exception == ADA_CONSTRAINT_ERROR && context.pendingMessage == message
            && context.pendingMessageLength == 11 && memcmp(message, "initializer", 11) == 0
            && context.pendingOrigin == origin && context.pendingTraceCount == count);
        clearException(ADA_CONSTRAINT_ERROR);
    }
    __ada_trace_leave_context(&context, &trace);
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    AdaFinalization* owner = NULL;
    void* arena = NULL;
    Pool pool = {0};
    AdaDispatchEntry operations[] = {
        {(void*)allocate, &pool}, {(void*)deallocate, &pool}, {(void*)storageSize, &pool}
    };
    AdaTag tag = {.slots = operations, .slotCount = 3};
    pool.tag = &tag;
    pool.reentrantCollection = __ada_collection_create(&owner, &arena);
    void* collection = __ada_collection_create(&owner, &arena);
    __ada_collection_set_pool(collection, &pool, 0, 1);
    CHECK(__ada_exception == NULL && __ada_collection_pool(collection, NULL) == &pool);
    CHECK(__ada_collection_storage_size(collection, NULL, 2) == (INT64_C(1) << 40));
    CHECK(atomic_load(&pool.queries) == 1);
    Worker worker = {collection};
    AdaThread threads[kWorkerCount];
    for (int i = 0; i < kWorkerCount; ++i) { CHECK(__ada_thread_create(&threads[i], exercise, &worker) == 0); }
    for (int i = 0; i < kWorkerCount; ++i) { CHECK(__ada_thread_join(threads[i], NULL) == 0); }
    CHECK(atomic_load(&pool.allocations) == kWorkerCount * kIterations);
    CHECK(atomic_load(&pool.allocations) == atomic_load(&pool.releases));
    pool.mode = 1;
    CHECK(__ada_collection_allocate_aligned(collection, 8, 8) == NULL);
    CHECK(__ada_task_context()->pendingMessageLength == 12);
    clearException(ADA_STORAGE_ERROR);
    pool.mode = 2;
    CHECK(__ada_collection_allocate_aligned(collection, 8, 8) == NULL);
    clearException(ADA_PROGRAM_ERROR);
    pool.mode = 3;
    CHECK(__ada_collection_allocate_aligned(collection, 8, 8) == NULL);
    clearException(ADA_PROGRAM_ERROR);
    pool.mode = 4;
    void* object = __ada_collection_allocate_aligned(collection, 8, 8);
    CHECK(object != NULL);
    __ada_deallocate(object);
    clearException(ADA_CONSTRAINT_ERROR);
    pool.mode = 0;
    // A pool callback can close the destination collection before returning.
    // Rejected registration must return its object to that same pool once.
    AdaFinalization* closingOwner = NULL;
    void* closingArena = NULL;
    void* closingCollection = __ada_collection_create(&closingOwner, &closingArena);
    __ada_collection_set_pool(closingCollection, &pool, 0, 1);
    pool.closeOwner = &closingOwner;
    pool.mode = 5;
    CHECK(__ada_collection_allocate_aligned(closingCollection, 17, 8) == NULL);
    clearException(ADA_PROGRAM_ERROR);
    CHECK(closingOwner == NULL && atomic_load(&pool.allocations) == atomic_load(&pool.releases));
    __ada_array_release(&closingArena);
    pool.closeOwner = NULL;
    pool.mode = 0;
    for (int i = 0; i < 10; ++i) { CHECK(__ada_collection_allocate_aligned(collection, i, 8) != NULL); }
    __ada_finalize_to(&owner, NULL);
    CHECK(__ada_exception == NULL && atomic_load(&pool.allocations) == atomic_load(&pool.releases));
    __ada_array_release(&arena);
    CHECK(__ada_task_context_dispose(__ada_task_context()));
    return 0;
}
