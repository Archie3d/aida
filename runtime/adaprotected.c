#include "adaprotected.h"
#include "adart.h"
#include "adathread.h"

#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(AdaProtectedAction) == 16, "protected action QBE ABI");

typedef struct AdaEntryWaiter
{
    struct AdaEntryWaiter* next;
    AdaProtectedBarrier barrier;
    void* link;
    void* object;
    int ready;
} AdaEntryWaiter;

typedef struct AdaProtectedLock
{
    AdaMutex mutex;
    AdaCondition changed;
    AdaEntryWaiter* queue;
    int servicing;
    int accessLevel;
    unsigned long epoch;
} AdaProtectedLock;

static void waitForChange(AdaProtectedLock* lock)
{
    if (__ada_condition_wait(&lock->changed, &lock->mutex) != 0) {
        abort();
    }
}

static void notify(AdaProtectedLock* lock)
{
    if (__ada_condition_broadcast(&lock->changed) != 0) {
        abort();
    }
}

/* A barrier runs in the servicing task's context with a clear exception
   status. Preserve the complete pending occurrence without allocation. */
static int evaluateBarrier(AdaTaskContext* context, AdaEntryWaiter* waiter)
{
    AdaTaskContext saved = *context;
    context->exception = NULL;
    context->pendingMessage = NULL;
    context->pendingMessageLength = 0;
    int open = waiter->barrier(waiter->link, waiter->object);
    int failed = context->exception != NULL;
    free(context->pendingMessage);
    context->exception = saved.exception;
    context->pendingMessage = saved.pendingMessage;
    context->pendingMessageLength = saved.pendingMessageLength;
    context->pendingOrigin = saved.pendingOrigin;
    context->pendingTraceCount = saved.pendingTraceCount;
    memcpy(context->pendingTrace, saved.pendingTrace, sizeof context->pendingTrace);
    return failed ? -1 : open != 0;
}

/* FIFO on each entry; among open entries choose the first queued caller.
   Only one caller receives the execution resource at a time. New external
   actions wait until the complete servicing chain has drained. */
static int serviceQueue(AdaTaskContext* context, AdaProtectedLock* lock)
{
    for (AdaEntryWaiter** slot = &lock->queue; *slot != NULL; slot = &(*slot)->next) {
        AdaEntryWaiter* waiter = *slot;
        int earlier = 0;
        for (AdaEntryWaiter* previous = lock->queue; previous != waiter; previous = previous->next) {
            earlier = earlier || previous->barrier == waiter->barrier;
        }
        if (earlier) {
            continue;
        }
        int open = evaluateBarrier(context, waiter);
        if (open < 0) {
            while (lock->queue != NULL) {
                AdaEntryWaiter* failed = lock->queue;
                lock->queue = failed->next;
                failed->ready = -1;
            }
            notify(lock);
            return 0;
        }
        if (open) {
            *slot = waiter->next;
            waiter->ready = 1;
            lock->servicing = 1;
            notify(lock);
            return 1;
        }
    }
    return 0;
}

void __ada_protected_init_access(void** object, int accessLevel)
{
    *object = NULL;
    AdaProtectedLock* lock = malloc(sizeof *lock);
    if (lock == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
        return;
    }
    if (__ada_mutex_init(&lock->mutex) != 0) {
        free(lock);
        __ada_raise(ADA_STORAGE_ERROR);
        return;
    }
    if (__ada_condition_init(&lock->changed) != 0) {
        __ada_mutex_destroy(&lock->mutex);
        free(lock);
        __ada_raise(ADA_STORAGE_ERROR);
        return;
    }
    lock->accessLevel = accessLevel;
    lock->queue = NULL;
    lock->servicing = 0;
    lock->epoch = 0;
    *object = lock;
}

void __ada_protected_destroy(void** object)
{
    AdaProtectedLock* lock = *object;
    if (lock != NULL) {
        if (lock->queue != NULL || lock->servicing
            || __ada_condition_destroy(&lock->changed) != 0
            || __ada_mutex_destroy(&lock->mutex) != 0) {
            __ada_raise(ADA_PROGRAM_ERROR);
            return;
        }
        free(lock);
        *object = NULL;
    }
}

void __ada_protected_enter(AdaTaskContext* context, void** object, AdaProtectedAction* action)
{
    AdaProtectedLock* lock = *object;
    if (lock == NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    for (AdaProtectedAction* held = context->protectedActions; held != NULL; held = held->previous) {
        if (held->lock == lock) {
            // An external call on the same object is a potentially blocking
            // operation. Internal calls never acquire another execution resource.
            __ada_raise(ADA_PROGRAM_ERROR);
            return;
        }
    }
    __ada_abort_defer(context);
    __ada_mutex_lock(&lock->mutex);
    while (lock->servicing) {
        waitForChange(lock);
    }
    action->lock = lock;
    action->previous = context->protectedActions;
    context->protectedActions = action;
}

void __ada_protected_leave(AdaTaskContext* context, AdaProtectedAction* action)
{
    if (context->protectedActions != action) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    context->protectedActions = action->previous;
    AdaProtectedLock* lock = action->lock;
    __ada_mutex_unlock(&lock->mutex);
    __ada_abort_undefer(context);
}

void __ada_protected_entry_enter(AdaTaskContext* context, void** object,
    AdaProtectedAction* action, AdaProtectedBarrier barrier, void* link)
{
    // Any entry call is potentially blocking, including calls to other objects.
    if (context->protectedActions != NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    __ada_protected_enter(context, object, action);
    if (context->exception != NULL) {
        return;
    }
    AdaProtectedLock* lock = action->lock;
    AdaEntryWaiter waiter = {NULL, barrier, link, object, 0};
    int open = evaluateBarrier(context, &waiter);
    if (open < 0) {
        while (lock->queue != NULL) {
            AdaEntryWaiter* failed = lock->queue;
            lock->queue = failed->next;
            failed->ready = -1;
        }
        notify(lock);
        __ada_protected_leave(context, action);
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    if (open && __ada_protected_count(object, barrier) == 0) {
        return;
    }
    AdaEntryWaiter** tail = &lock->queue;
    while (*tail != NULL) {
        tail = &(*tail)->next;
    }
    *tail = &waiter;
    serviceQueue(context, lock);
    context->protectedActions = action->previous;
    __ada_abort_undefer(context);
    while (waiter.ready == 0) {
        waitForChange(lock);
    }
    if (waiter.ready < 0) {
        __ada_mutex_unlock(&lock->mutex);
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    __ada_abort_defer(context);
    context->protectedActions = action;
}

void __ada_protected_complete(AdaTaskContext* context, AdaProtectedAction* action)
{
    if (context->protectedActions != action) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    AdaProtectedLock* lock = action->lock;
    lock->servicing = 0;
    unsigned long epoch = lock->epoch;
    int selected = serviceQueue(context, lock);
    context->protectedActions = action->previous;
    if (selected) {
        // The selected caller executes in its own task context. Wait for the
        // last eligible entry to end this servicing chain before returning;
        // servicing keeps new external actions from acquiring the resource.
        while (lock->epoch == epoch) {
            waitForChange(lock);
        }
    } else {
        ++lock->epoch;
        notify(lock);
    }
    __ada_mutex_unlock(&lock->mutex);
    __ada_abort_undefer(context);
}

int __ada_protected_count(void** object, AdaProtectedBarrier barrier)
{
    AdaProtectedLock* lock = *object;
    int count = 0;
    for (AdaEntryWaiter* waiter = lock->queue; waiter != NULL; waiter = waiter->next) {
        count += waiter->barrier == barrier;
    }
    return count;
}

void __ada_protected_init(void** object)
{
    int level = __ada_current_access_level();
    if (__ada_exception == NULL) { __ada_protected_init_access(object, level); }
}

int __ada_protected_access_level(void** object)
{
    return ((AdaProtectedLock*)*object)->accessLevel;
}
