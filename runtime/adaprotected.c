#include "adaprotected.h"
#include "adart.h"
#include "adathread.h"
#include "adatime.h"
#include "adaentry.h"

#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>

_Static_assert(sizeof(AdaProtectedAction) == kAdaProtectedActionBytes, "protected action QBE ABI");

static atomic_int lockingPolicy = kAdaLockingDefault;

void __ada_locking_policy(enum AdaLockingPolicy policy)
{
    if (policy != kAdaCeilingLocking && policy != kAdaNativeLocking) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    int expected = kAdaLockingDefault;
    if (!atomic_compare_exchange_strong(&lockingPolicy, &expected, policy) && expected != policy) {
        __ada_raise(ADA_PROGRAM_ERROR);
    }
}

typedef struct AdaEntryWaiter
{
    struct AdaEntryWaiter* next;
    AdaProtectedBarrier barrier;
    void* link;
    void* object;
    int ready;
    AdaEntryCall* call;
} AdaEntryWaiter;

typedef struct AdaProtectedLock
{
    AdaMutex mutex;
    AdaCondition changed;
    AdaEntryWaiter* queue;
    int servicing;
    int accessLevel;
    int ceiling;
    unsigned long epoch;
    atomic_uint references;
    unsigned int pending;
    int disposed;
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

typedef struct AdaProtectedPendingException
{
    const AdaException* identity;
    char* message;
    int length;
    const char* origin;
    int traceCount;
    AdaTraceEntry trace[ADA_TRACE_CAPACITY];
} AdaProtectedPendingException;

static AdaProtectedPendingException detachException(AdaTaskContext* context)
{
    AdaProtectedPendingException saved = {context->exception, context->pendingMessage,
        context->pendingMessageLength, context->pendingOrigin, context->pendingTraceCount, {{0}}};
    memcpy(saved.trace, context->pendingTrace, sizeof saved.trace);
    context->exception = NULL;
    context->pendingMessage = NULL;
    context->pendingMessageLength = 0;
    return saved;
}

static void restoreException(AdaTaskContext* context, const AdaProtectedPendingException* saved)
{
    free(context->pendingMessage);
    context->exception = saved->identity;
    context->pendingMessage = saved->message;
    context->pendingMessageLength = saved->length;
    context->pendingOrigin = saved->origin;
    context->pendingTraceCount = saved->traceCount;
    memcpy(context->pendingTrace, saved->trace, sizeof context->pendingTrace);
}

/* A barrier runs with clear status; preserve the complete pending occurrence
   without allocation, including a message owned by an interrupted caller. */
static int evaluateBarrier(AdaTaskContext* context, AdaEntryWaiter* waiter)
{
    AdaProtectedPendingException saved = detachException(context);
    int open = waiter->barrier(waiter->link, waiter->object);
    int failed = context->exception != NULL;
    restoreException(context, &saved);
    return failed ? -1 : open != 0;
}

static void failQueue(AdaProtectedLock* lock)
{
    while (lock->queue != NULL) {
        AdaEntryWaiter* failed = lock->queue;
        lock->queue = failed->next;
        failed->ready = -1;
        if (failed->call != NULL) {
            __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
            failed->call->protectedWaiter = NULL;
            __ada_entry_finish_locked(failed->call, ADA_PROGRAM_ERROR);
            __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
            free(failed);
        }
    }
    notify(lock);
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
            failQueue(lock);
            return 0;
        }
        if (open) {
            *slot = waiter->next;
            waiter->ready = 1;
            lock->servicing = 1;
            if (waiter->call != NULL) {
                __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
                __ada_entry_selected_locked(waiter->call);
                __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
            }
            notify(lock);
            return 1;
        }
    }
    return 0;
}

typedef struct AdaProtectedQueueService
{
    AdaProtectedPendingException pending;
    AdaProtectedAction action;
} AdaProtectedQueueService;

/* Removing a queued caller can require barrier evaluation. It is a protected
   action too; its priority must be restored only after releasing the mutex. */
static void beginQueueService(AdaTaskContext* context, AdaProtectedLock* lock,
    AdaProtectedQueueService* service)
{
    service->pending = detachException(context);
    service->action = (AdaProtectedAction) {lock, context->protectedActions,
        __ada_priority_enter(lock->ceiling)};
    if (service->action.savedPriority < 0) {
        failQueue(lock);
        return;
    }
    context->protectedActions = &service->action;
    serviceQueue(context, lock);
    context->protectedActions = service->action.previous;
}

static void finishQueueService(AdaTaskContext* context, AdaProtectedQueueService* service)
{
    if (service->action.savedPriority >= 0) { __ada_priority_leave(service->action.savedPriority); }
    if (service->pending.identity != NULL || context->exception == NULL) {
        restoreException(context, &service->pending);
    } else {
        free(service->pending.message);
    }
}

void __ada_protected_init_access(AdaConcurrentObject* object, int accessLevel)
{
    object->resource = NULL;
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
    lock->ceiling = atomic_load(&lockingPolicy) == kAdaCeilingLocking ? kAdaPriorityLast : -1;
    lock->queue = NULL;
    lock->servicing = 0;
    lock->epoch = 0;
    atomic_init(&lock->references, 1);
    lock->pending = 0;
    lock->disposed = 0;
    object->resource = lock;
}

void __ada_protected_set_ceiling(AdaConcurrentObject* object, int priority)
{
    if (object->resource == NULL || priority < kAdaPriorityFirst || priority > kAdaAnyPriorityLast) {
        __ada_raise(ADA_CONSTRAINT_ERROR); return;
    }
    ((AdaProtectedLock*)object->resource)->ceiling = priority;
}

void __ada_protected_set_interrupt_ceiling(AdaConcurrentObject* object, int priority)
{
    if (priority <= kAdaPriorityLast || priority > kAdaAnyPriorityLast) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return;
    }
    __ada_protected_set_ceiling(object, priority);
}

static void releaseLock(AdaProtectedLock* lock)
{
    if (atomic_fetch_sub(&lock->references, 1) == 1) {
        if (__ada_condition_destroy(&lock->changed) != 0 || __ada_mutex_destroy(&lock->mutex) != 0) { abort(); }
        free(lock);
    }
}

void* __ada_protected_target_retain(AdaConcurrentObject* object)
{
    AdaProtectedLock* lock = object->resource;
    if (lock == NULL) { __ada_raise(ADA_PROGRAM_ERROR); return NULL; }
    // The entry name is evaluated while the source callable still exists.
    // Pin its destination through source finalization and queue publication.
    atomic_fetch_add(&lock->references, 1);
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    ++lock->pending;
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    return lock;
}

void __ada_protected_target_release_locked(void* pointer, int pending)
{
    if (pointer == NULL) { return; }
    AdaProtectedLock* lock = pointer;
    if (pending) {
        --lock->pending;
        __ada_condition_broadcast(&__ada_main_context.sharedMasterChanged);
    }
    releaseLock(lock);
}

void __ada_protected_target_release(void* pointer, int pending)
{
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    __ada_protected_target_release_locked(pointer, pending);
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
}

void __ada_protected_destroy(AdaConcurrentObject* object)
{
    AdaProtectedLock* lock = object->resource;
    if (lock == NULL) { return; }
    for (AdaProtectedAction* held = __ada_task_context()->protectedActions; held != NULL; held = held->previous) {
        if (held->lock == lock) { __ada_raise(ADA_PROGRAM_ERROR); return; }
    }
    __ada_mutex_lock(&lock->mutex);
    for (;;) {
        while (lock->servicing) { waitForChange(lock); }
        __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
        if (lock->pending == 0) { break; }
        __ada_mutex_unlock(&lock->mutex);
        while (lock->pending != 0) {
            __ada_condition_wait(&__ada_main_context.sharedMasterChanged, &__ada_main_context.sharedMasterMutex);
        }
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        __ada_mutex_lock(&lock->mutex);
    }
    lock->disposed = 1;
    while (lock->queue != NULL) {
        AdaEntryWaiter* waiter = lock->queue;
        lock->queue = waiter->next;
        waiter->ready = -1;
        if (waiter->call != NULL) {
            waiter->call->protectedWaiter = NULL;
            __ada_entry_finish_locked(waiter->call, ADA_PROGRAM_ERROR);
            free(waiter);
        }
    }
    object->resource = NULL;
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    notify(lock);
    __ada_mutex_unlock(&lock->mutex);
    releaseLock(lock);
}

void __ada_protected_enter(AdaTaskContext* context, AdaConcurrentObject* object, AdaProtectedAction* action)
{
    AdaProtectedLock* lock = object->resource;
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
    if (__ada_priority_check(lock->ceiling) != 0) { return; }
    // An action retains its resource while restoring priority outside the mutex.
    atomic_fetch_add(&lock->references, 1);
    __ada_abort_defer(context);
    __ada_mutex_lock(&lock->mutex);
    while (lock->servicing && !lock->disposed) {
        waitForChange(lock);
    }
    if (lock->disposed) {
        __ada_mutex_unlock(&lock->mutex);
        releaseLock(lock);
        __ada_raise(ADA_PROGRAM_ERROR);
        __ada_abort_undefer(context);
        return;
    }
    action->savedPriority = __ada_priority_enter(lock->ceiling);
    if (action->savedPriority < 0) {
        __ada_mutex_unlock(&lock->mutex);
        releaseLock(lock);
        __ada_abort_undefer(context);
        return;
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
    __ada_priority_leave(action->savedPriority);
    releaseLock(lock);
    __ada_abort_undefer(context);
}

int __ada_protected_entry_enter_until(AdaTaskContext* context, AdaConcurrentObject* object,
    AdaProtectedAction* action, AdaProtectedBarrier barrier, void* link,
    int64_t deadline, enum AdaSelectClock clock, enum AdaEntryCallMode mode)
{
    // Any entry call is potentially blocking, including calls to other objects.
    if (context->protectedActions != NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return 0;
    }
    __ada_protected_enter(context, object, action);
    if (context->exception != NULL) {
        return 0;
    }
    AdaProtectedLock* lock = action->lock;
    AdaEntryWaiter waiter = {NULL, barrier, link, object, 0};
    int open = evaluateBarrier(context, &waiter);
    if (open < 0) {
        failQueue(lock);
        __ada_protected_leave(context, action);
        __ada_raise(ADA_PROGRAM_ERROR);
        return 0;
    }
    if (open && __ada_protected_count(object, barrier) == 0) {
        return 1;
    }
    AdaEntryWaiter** tail = &lock->queue;
    while (*tail != NULL) {
        tail = &(*tail)->next;
    }
    *tail = &waiter;
    atomic_fetch_add(&lock->references, 1);
    serviceQueue(context, lock);
    context->protectedActions = action->previous;
    __ada_mutex_unlock(&lock->mutex);
    __ada_priority_leave(action->savedPriority);
    __ada_abort_undefer(context);
    __ada_mutex_lock(&lock->mutex);
    while (waiter.ready == 0) {
        __ada_mutex_unlock(&lock->mutex);
        __ada_async_poll(context);
        __ada_mutex_lock(&lock->mutex);
        if (waiter.ready != 0) {
            if (context->exception == &__ada_exc_async_transfer) { context->exception = NULL; }
            break;
        }
        int64_t until = mode == ADA_ENTRY_WAIT ? INT64_MAX : __ada_select_wait_deadline(deadline, clock);
        if (context->exception != NULL || mode == ADA_ENTRY_CONDITIONAL || (mode == ADA_ENTRY_TIMED && until <= __ada_real_clock())) {
            AdaEntryWaiter** slot = &lock->queue;
            while (*slot != NULL && *slot != &waiter) { slot = &(*slot)->next; }
            if (*slot != NULL) { *slot = waiter.next; }
            AdaProtectedQueueService service;
            int serviceNeeded = !lock->servicing;
            if (serviceNeeded) { beginQueueService(context, lock, &service); }
            notify(lock);
            __ada_mutex_unlock(&lock->mutex);
            if (serviceNeeded) { finishQueueService(context, &service); }
            releaseLock(lock);
            releaseLock(lock);
            return 0;
        }
        int64_t poll = __ada_real_clock() + 10000000;
        if (until > poll) { until = poll; }
        __ada_condition_wait_until(&lock->changed, &lock->mutex, until);
    }
    if (waiter.ready < 0) {
        __ada_mutex_unlock(&lock->mutex);
        releaseLock(lock);
        releaseLock(lock);
        __ada_raise(ADA_PROGRAM_ERROR);
        return 0;
    }
    releaseLock(lock);
    __ada_abort_defer(context);
    action->savedPriority = __ada_priority_enter(lock->ceiling);
    context->protectedActions = action;
    if (action->savedPriority < 0) {
        __ada_protected_complete(context, action);
        return 0;
    }
    return 1;
}

void __ada_protected_entry_enter(AdaTaskContext* context, AdaConcurrentObject* object,
    AdaProtectedAction* action, AdaProtectedBarrier barrier, void* link)
{
    __ada_protected_entry_enter_until(context, object, action, barrier, link,
        0, ADA_SELECT_MONOTONIC, ADA_ENTRY_WAIT);
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
    int selected = 0;
    if (action->savedPriority < 0) { failQueue(lock); }
    else { selected = serviceQueue(context, lock); }
    context->protectedActions = action->previous;
    if (!selected) {
        ++lock->epoch;
        notify(lock);
    }
    __ada_mutex_unlock(&lock->mutex);
    __ada_priority_leave(action->savedPriority);
    if (selected) {
        __ada_mutex_lock(&lock->mutex);
        // The selected caller executes in its own task context. Wait for the
        // last eligible entry to end this servicing chain before returning;
        // servicing keeps new external actions from acquiring the resource.
        while (lock->epoch == epoch) {
            waitForChange(lock);
        }
        __ada_mutex_unlock(&lock->mutex);
    }
    releaseLock(lock);
    __ada_abort_undefer(context);
}

int __ada_protected_count(AdaConcurrentObject* object, AdaProtectedBarrier barrier)
{
    AdaProtectedLock* lock = object->resource;
    int count = 0;
    for (AdaEntryWaiter* waiter = lock->queue; waiter != NULL; waiter = waiter->next) {
        count += waiter->barrier == barrier;
    }
    return count;
}

void __ada_protected_init(AdaConcurrentObject* object)
{
    int level = __ada_current_access_level();
    if (__ada_exception == NULL) { __ada_protected_init_access(object, level); }
}

int __ada_protected_access_level(AdaConcurrentObject* object)
{
    return ((AdaProtectedLock*)object->resource)->accessLevel;
}

/* Shared-call entry execution. Legacy enter/complete callers retain their
   caller-owned action ABI; generated entry calls use these queue nodes. */
void __ada_protected_request_enqueue(AdaTaskContext* context, AdaEntryCall* call, int internal)
{
    AdaEntryWaiter* waiter = calloc(1, sizeof *waiter);
    if (waiter == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
        __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
        __ada_entry_finish_locked(call, ADA_STORAGE_ERROR);
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        return;
    }
    AdaProtectedAction action;
    AdaProtectedLock* lock = call->protectedLock;
    if (!internal) {
        if (__ada_priority_check(lock->ceiling) != 0) {
            __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
            if (call->protectedPending) { --lock->pending; call->protectedPending = 0; }
            __ada_entry_finish_locked(call, context->exception);
            __ada_condition_broadcast(&__ada_main_context.sharedMasterChanged);
            __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
            free(waiter);
            return;
        }
        __ada_abort_defer(context);
        __ada_mutex_lock(&lock->mutex);
        while (lock->servicing && !lock->disposed) { waitForChange(lock); }
        action = (AdaProtectedAction) {lock, context->protectedActions, __ada_priority_enter(lock->ceiling)};
        if (action.savedPriority < 0) {
            __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
            if (call->protectedPending) { --lock->pending; call->protectedPending = 0; }
            __ada_entry_finish_locked(call, context->exception);
            __ada_condition_broadcast(&__ada_main_context.sharedMasterChanged);
            __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
            __ada_mutex_unlock(&lock->mutex);
            __ada_abort_undefer(context);
            free(waiter);
            return;
        }
        context->protectedActions = &action;
    }
    *waiter = (AdaEntryWaiter) {NULL, call->barrier, call->link, call->object, 0, call};
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    if (call->protectedPending) {
        --lock->pending;
        call->protectedPending = 0;
        __ada_condition_broadcast(&__ada_main_context.sharedMasterChanged);
    }
    if (lock->disposed) {
        __ada_entry_finish_locked(call, ADA_PROGRAM_ERROR);
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        free(waiter);
    } else if (call->cancelRequested && call->abortable) {
        __ada_entry_cancel_locked(call);
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        free(waiter);
    } else {
        call->protectedWaiter = waiter;
        call->registered = 1;
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        // A new external call checks its barrier before joining the queue.
        // Count-dependent barriers must not include this call yet. Internal
        // requeues join first and are reconsidered when the action completes.
        int open = internal ? 0 : evaluateBarrier(context, waiter);
        if (open < 0) {
            failQueue(lock);
            __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
            call->protectedWaiter = NULL;
            __ada_entry_finish_locked(call, ADA_PROGRAM_ERROR);
            __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
            free(waiter);
        } else if (open && __ada_protected_count(call->object, call->barrier) == 0) {
            lock->servicing = 1;
            waiter->ready = 1;
            __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
            __ada_entry_selected_locked(call);
            __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        } else {
            AdaEntryWaiter** tail = &lock->queue;
            while (*tail != NULL) { tail = &(*tail)->next; }
            *tail = waiter;
            if (!internal) { serviceQueue(context, lock); }
        }
    }
    if (!internal) {
        context->protectedActions = action.previous;
        __ada_mutex_unlock(&lock->mutex);
        __ada_priority_leave(action.savedPriority);
        __ada_abort_undefer(context);
    }
}

static void releaseRequeueAction(AdaTaskContext* context, AdaProtectedAction* action)
{
    AdaProtectedLock* lock = action->lock;
    lock->servicing = 0;
    if (!serviceQueue(context, lock)) { ++lock->epoch; notify(lock); }
    context->protectedActions = action->previous;
    __ada_mutex_unlock(&lock->mutex);
    __ada_priority_leave(action->savedPriority);
    releaseLock(lock);
    __ada_abort_undefer(context);
}

void __ada_protected_request_execute(AdaTaskContext* context, AdaEntryCall* call)
{
    AdaProtectedLock* lock = call->protectedLock;
    atomic_fetch_add(&lock->references, 1);
    __ada_mutex_lock(&lock->mutex);
    AdaProtectedAction action = {lock, context->protectedActions, __ada_priority_enter(lock->ceiling)};
    context->protectedActions = &action;
    __ada_abort_defer(context);
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    AdaEntryWaiter* waiter = call->protectedWaiter;
    call->protectedWaiter = NULL;
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    free(waiter);
    AdaEntryCall* previous = context->currentEntry;
    context->currentEntry = call;
    if (context->exception == NULL) { call->body(call->arguments); }
    context->currentEntry = previous;
    if (context->exception == NULL && call->redirect != NULL) {
        __ada_entry_complete(context, call, 1);
        releaseRequeueAction(context, &action);
    } else {
        __ada_protected_complete(context, &action);
        __ada_entry_complete(context, call, 0);
    }
}

int __ada_protected_request_cancel(AdaEntryCall* call)
{
    AdaTaskContext* context = __ada_task_context();
    __ada_abort_defer(context);
    AdaProtectedLock* lock = call->protectedLock;
    atomic_fetch_add(&lock->references, 1);
    __ada_mutex_lock(&lock->mutex);
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    if (call->selected || call->done || !call->abortable) {
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        __ada_mutex_unlock(&lock->mutex);
        releaseLock(lock);
        __ada_abort_undefer(context);
        return 0;
    }
    AdaEntryWaiter* waiter = call->protectedWaiter;
    AdaEntryWaiter** slot = &lock->queue;
    while (*slot != NULL && *slot != waiter) { slot = &(*slot)->next; }
    if (*slot != NULL) { *slot = waiter->next; }
    call->protectedWaiter = NULL;
    __ada_entry_cancel_locked(call);
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    free(waiter);
    AdaProtectedQueueService service;
    int serviceNeeded = !lock->servicing;
    if (serviceNeeded) { beginQueueService(context, lock, &service); }
    notify(lock);
    __ada_mutex_unlock(&lock->mutex);
    if (serviceNeeded) { finishQueueService(context, &service); }
    releaseLock(lock);
    __ada_abort_undefer(context);
    return 1;
}

int __ada_protected_callback_equal(const void* left, const void* right)
{
    if (left == right) { return 1; }
    if (left == NULL || right == NULL) { return 0; }
    // The established descriptor ABI is code, lexical link, target object.
    enum { kFields = 3 };
    const void* a[kFields];
    const void* b[kFields];
    memcpy(a, left, sizeof a);
    memcpy(b, right, sizeof b);
    for (unsigned i = 0; i < kFields; ++i) {
        if (a[i] != b[i]) { return 0; }
    }
    return 1;
}
