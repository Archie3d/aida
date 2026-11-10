#include "adamaster.h"
#include "adart.h"
#include "adathread.h"
#include "adatime.h"
#include "adacalendar.h"
#include "adaprotected.h"
#include "adaentry.h"
#include "adataskservices.h"
#include <errno.h>
#include <stddef.h>

#include <stdlib.h>
#include <stdatomic.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

_Static_assert(sizeof(AdaSelectChoice) == kAdaSelectChoiceBytes, "select alternative QBE ABI");
_Static_assert(offsetof(AdaSelectChoice, index) == 8 && offsetof(AdaSelectChoice, deadline) == 16
    && offsetof(AdaSelectChoice, clock) == 24 && offsetof(AdaSelectChoice, open) == 28, "select alternative fields");

_Static_assert(sizeof(AdaConcurrentObject) == kAdaConcurrentHeaderBytes
    && offsetof(AdaConcurrentObject, resource) == 8, "concurrent object QBE ABI");

_Static_assert(sizeof(AdaMaster) == kAdaMasterBytes, "master record QBE ABI");

enum
{
    kMasterRegistered = 1,
    kMasterDependents = 2,
    kMasterAccessLevel = 4,
    kMasterCompleting = 8
};

/* Parent links remain live until dependent tasks and finalization finish.
   Shared collection reparenting and task publication hold the master mutex. */
static void markDependents(AdaMaster* master)
{
    for (; master != NULL; master = master->state) {
        atomic_fetch_or_explicit(&master->activities, kMasterDependents, memory_order_release);
    }
}

static unsigned int masterActivities(AdaMaster* master)
{
    return master != NULL ? atomic_load_explicit(&master->activities, memory_order_acquire) : 0;
}

typedef AdaEntryCall AdaRendezvous;

typedef struct AdaTaskState
{
    struct AdaTaskState* next;
    void* object;
    void (*body)(void*);
    void* link;
    void* group;
    const int* elaborated;
    AdaThread thread;
    AdaPriorityThread priorityThread;
    atomic_int basePriority;
    int priorityReady;
    int activationPriority;
    int64_t storageSize;
    int stackSpecified;
    AdaMutex mutex;
    AdaCondition condition;
    AdaCondition entryChanged;
    const AdaSelectChoice* choices;
    int choiceCount;
    int waitingTerminate;
    int terminateSelected;
    AdaRendezvous* selected;
    AdaMaster* master;
    AdaRendezvous* calls;
    AdaRendezvous* accepted;
    size_t activeCalls;
    int started;
    int activating;
    int joining;
    int activated;
    int activationFailed;
    int joined;
    int blocked;
    AdaRendezvous* blockedCall;
    AdaEntryCall* outgoing;
    atomic_int abortRequested;
    atomic_int completed;
    atomic_int terminated;
    char image[64];
    AdaTaskContext context;
} AdaTaskState;

typedef struct AdaMasterState
{
    struct AdaMasterState* next;
    struct AdaMasterState* registeredNext;
    AdaTaskContext* owner;
    AdaMaster* parent;
    AdaMaster* master;
    AdaTaskState* tasks;
    int collection;
    AdaTaskState* ownerTask;
} AdaMasterState;

static void checkTerminateCohorts(void);
static void freeEntryCall(AdaEntryCall* call);

enum AdaAsyncKind
{
    kAsyncDelay,
    kAsyncTask,
    kAsyncProtected
};

typedef struct AdaAsyncSelect
{
    struct AdaAsyncSelect* previous;
    enum AdaAsyncKind kind;
    unsigned int deferralDepth;
    int64_t deadline;
    enum AdaSelectClock clock;
    int immediate;
    void* request;
    AdaTaskState* server;
} AdaAsyncSelect;

const AdaException __ada_exc_async_transfer = { "__ASYNC_TRANSFER" };

static AdaTaskState environmentTask = { .image = "environment",
    .mutex = ADA_MUTEX_INITIALIZER, .basePriority = kAdaDefaultPriority };

void* __ada_current_task(void)
{
    AdaTaskContext* context = __ada_task_context();
    return context->taskSelf != NULL ? context->taskSelf : &environmentTask;
}

int __ada_get_priority(void* identity)
{
    if (identity == NULL) { __ada_raise(ADA_PROGRAM_ERROR); return 0; }
    AdaTaskState* task = identity;
    if (atomic_load(&task->terminated)) { __ada_raise(ADA_TASKING_ERROR); return 0; }
    return atomic_load(&task->basePriority);
}

static int activePriority(AdaTaskState* task, AdaTaskContext* context)
{
    int base = atomic_load(&task->basePriority);
    int floor = atomic_load(&context->inheritedPriority) - 1;
    return base > floor ? base : floor;
}

void __ada_set_priority(int priority, void* identity)
{
    if (identity == NULL) { __ada_raise(ADA_PROGRAM_ERROR); return; }
    if (priority < kAdaPriorityFirst || priority > kAdaAnyPriorityLast) {
        __ada_raise(ADA_CONSTRAINT_ERROR); return;
    }
    AdaTaskState* task = identity;
    __ada_mutex_lock(&task->mutex);
    int status = 0;
    if (!atomic_load(&task->terminated)) {
        AdaTaskContext* context = task == &environmentTask ? &__ada_main_context : &task->context;
        int floor = atomic_load(&context->inheritedPriority) - 1;
        if (!task->priorityReady && task == __ada_current_task()) {
            task->priorityThread = __ada_priority_thread_self();
            task->priorityReady = 1;
        }
        if (task->priorityReady) {
            status = __ada_thread_set_priority(task->priorityThread, priority > floor ? priority : floor);
        }
        if (status == 0) { atomic_store(&task->basePriority, priority); }
    }
    __ada_mutex_unlock(&task->mutex);
    if (status != 0) { __ada_raise(ADA_TASKING_ERROR); }
}

void __ada_main_priority(int priority)
{
    __ada_set_priority(priority, __ada_current_task());
}

static int refreshPriority(AdaTaskContext* context)
{
    AdaTaskState* task = __ada_current_task();
    int priority = activePriority(task, context);
    if (context->priorityApplied && priority == context->appliedPriority) { return 0; }
    __ada_mutex_lock(&task->mutex);
    priority = activePriority(task, context);
    int status = __ada_thread_set_priority(__ada_priority_thread_self(), priority);
    if (status == 0) { context->priorityApplied = 1; context->appliedPriority = priority; }
    __ada_mutex_unlock(&task->mutex);
    if (status != 0 && context->exception == NULL) { __ada_raise(ADA_TASKING_ERROR); }
    return status;
}

int __ada_priority_inherit(int priority)
{
    AdaTaskContext* context = __ada_task_context();
    int saved = atomic_load(&context->inheritedPriority);
    if (priority + 1 > saved) { atomic_store(&context->inheritedPriority, priority + 1); }
    if (refreshPriority(context) != 0) { atomic_store(&context->inheritedPriority, saved); return -1; }
    return saved;
}

int __ada_priority_enter(int ceiling)
{
    AdaTaskContext* context = __ada_task_context();
    int saved = atomic_load(&context->inheritedPriority);
    if (ceiling < 0) { return saved; }
    if (activePriority(__ada_current_task(), context) > ceiling) {
        __ada_raise(ADA_PROGRAM_ERROR); return -1;
    }
    return __ada_priority_inherit(ceiling);
}

void __ada_priority_leave(int saved)
{
    if (saved < 0) { return; }
    AdaTaskContext* context = __ada_task_context();
    atomic_store(&context->inheritedPriority, saved);
    refreshPriority(context);
}

int64_t __ada_task_storage_size(void* identity)
{
    if (identity == NULL) { __ada_raise(ADA_PROGRAM_ERROR); return 0; }
    return ((AdaTaskState*)identity)->storageSize;
}

void* __ada_task_identity(AdaConcurrentObject* object)
{
    if (object->resource == NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
    }
    return object->resource;
}

void* __ada_current_task_object(void)
{
    AdaTaskState* task = __ada_task_context()->taskSelf;
    if (task == NULL) { __ada_raise(ADA_PROGRAM_ERROR); return NULL; }
    return task->object;
}

int __ada_task_id_equal(void* left, void* right)
{
    return left == right;
}

const char* __ada_task_image(void* identity)
{
    return identity != NULL ? ((AdaTaskState*)identity)->image : "";
}

int __ada_task_is_callable(void* identity)
{
    if (identity == NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return 0;
    }
    AdaTaskState* task = identity;
    return !atomic_load(&task->completed) && !atomic_load(&task->abortRequested);
}

int __ada_task_is_terminated(void* identity)
{
    if (identity == NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return 0;
    }
    return atomic_load(&((AdaTaskState*)identity)->terminated);
}

static void completeTaskLocked(AdaTaskState* task)
{
    atomic_store(&task->completed, 1);
    for (AdaRendezvous* call = task->calls; call != NULL;) {
        AdaRendezvous* next = call->next;
        call->next = NULL;
        __ada_entry_finish_locked(call, ADA_TASKING_ERROR);
        call = next;
    }
    task->calls = NULL;
    __ada_condition_broadcast(&task->entryChanged);
}

static void completeTask(AdaTaskState* task)
{
    if (task == NULL) { return; }
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    completeTaskLocked(task);
    checkTerminateCohorts();
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
}

void __ada_task_complete(AdaTaskContext* context)
{
    completeTask(context->taskSelf);
}

int64_t __ada_select_deadline(int64_t value, enum AdaSelectClock clock)
{
    if (clock != ADA_SELECT_RELATIVE) { return value; }
    int64_t now = __ada_real_clock();
    if (value <= 0) { return now; }
    __int128 span = ((__int128)value * INT64_C(1000000000) + ADA_DURATION_SCALE - 1) / ADA_DURATION_SCALE;
    __int128 deadline = now + span;
    if (deadline > INT64_MAX) { __ada_raise(ADA_CONSTRAINT_ERROR); return 0; }
    return (int64_t)deadline;
}

int64_t __ada_select_wait_deadline(int64_t value, enum AdaSelectClock clock)
{
    if (clock != ADA_SELECT_CALENDAR) { return value; }
    int64_t now = __ada_real_clock();
    int64_t calendar = __ada_calendar_clock();
    __int128 remaining = ((__int128)value - calendar) * 1000;
    if (remaining <= 0) { return now; }
    // Recheck wall-clock deadlines at least once a second after clock changes.
    if (remaining > INT64_C(1000000000)) { remaining = INT64_C(1000000000); }
    return now > INT64_MAX - remaining ? INT64_MAX : now + (int64_t)remaining;
}

static int expired(int64_t deadline, enum AdaSelectClock clock)
{
    return clock == ADA_SELECT_CALENDAR ? __ada_calendar_clock() >= deadline : __ada_real_clock() >= deadline;
}

/* The shared master mutex serializes registration, commitment and cancellation.
   A descriptor is visible only while its accepting task is blocked here. */
void* __ada_async_start_delay(AdaTaskContext* context, int64_t deadline, enum AdaSelectClock clock)
{
    if (context->abortDeferralDepth != 0) { __ada_raise(ADA_PROGRAM_ERROR); return NULL; }
    AdaAsyncSelect* operation = calloc(1, sizeof *operation);
    if (operation == NULL) { __ada_raise(ADA_STORAGE_ERROR); return NULL; }
    operation->kind = kAsyncDelay;
    operation->deadline = deadline;
    operation->clock = clock;
    operation->deferralDepth = context->abortDeferralDepth;
    operation->immediate = expired(deadline, clock);
    if (context->exception != NULL) { free(operation); return NULL; }
    operation->previous = context->asyncSelects;
    context->asyncSelects = operation;
    return operation;
}

int __ada_async_run(void* pointer)
{
    AdaAsyncSelect* operation = pointer;
    if (!operation->immediate) { return 1; }
    if (operation->kind == kAsyncDelay) { return 0; }
    AdaEntryCall* call = operation->request;
    AdaTaskContext* context = __ada_task_context();
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    // Immediate selection skips the part only if the call is never requeued
    // with abort. Wait for completion or that first cancellable handoff.
    while (!call->done && !call->requeuedWithAbort) {
        __ada_condition_wait_until(&call->changed, &__ada_main_context.sharedMasterMutex,
            __ada_real_clock() + INT64_C(10000000));
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        __ada_async_poll(context);
        __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
        if (context->exception != NULL) { call->cancelRequested = 1; }
    }
    int run = !call->done;
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    return run;
}

static int asyncCompleted(AdaAsyncSelect* operation)
{
    if (operation->kind == kAsyncDelay) { return expired(operation->deadline, operation->clock); }
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    int done = ((AdaRendezvous*)operation->request)->done;
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    return done;
}

int __ada_async_ready(AdaTaskContext* context)
{
    if (context->abortDeferralDepth == 0
        && atomic_load(&(context->taskSelf != NULL ? context->taskSelf : &environmentTask)->abortRequested)) { return 1; }
    for (AdaAsyncSelect* operation = context->asyncSelects; operation != NULL; operation = operation->previous) {
        if (context->abortDeferralDepth <= operation->deferralDepth && asyncCompleted(operation)) { return 1; }
    }
    return 0;
}

void __ada_async_poll(AdaTaskContext* context)
{
    if (context->exception != NULL) { return; }
    refreshPriority(context);
    if (context->exception != NULL) { return; }
    if (context->abortDeferralDepth == 0
        && atomic_load(&(context->taskSelf != NULL ? context->taskSelf : &environmentTask)->abortRequested)) {
        context->asyncTarget = NULL;
        __ada_raise(&__ada_exc_async_transfer);
        return;
    }
    for (AdaAsyncSelect* operation = context->asyncSelects; operation != NULL; operation = operation->previous) {
        if (context->abortDeferralDepth <= operation->deferralDepth && asyncCompleted(operation)) {
            context->asyncTarget = operation;
            __ada_raise(&__ada_exc_async_transfer);
            return;
        }
    }
}

int __ada_async_check(AdaTaskContext* context)
{
    __ada_async_poll(context);
    return context->exception == &__ada_exc_async_transfer;
}

int __ada_async_matches(AdaTaskContext* context, void* operation)
{
    return context->exception == &__ada_exc_async_transfer && context->asyncTarget == operation;
}

int __ada_async_leave(AdaTaskContext* context, void* pointer)
{
    AdaAsyncSelect* operation = pointer;
    if (context->asyncSelects != operation) { abort(); }
    int selected = asyncCompleted(operation);
    int matched = __ada_async_matches(context, operation);
    context->asyncSelects = operation->previous;
    if (matched) {
        context->exception = NULL;
        context->asyncTarget = NULL;
    }
    if (operation->kind != kAsyncDelay) {
        AdaEntryCall* call = operation->request;
        __ada_entry_cancel_request(call);
        __ada_thread_join(call->worker, NULL);
        selected = !call->cancelled;
        if (context->exception == NULL && call->exception.identity != NULL) { __ada_reraise(&call->exception); }
        call->workerContext.taskSelf = NULL;
        call->workerContext.currentMaster = NULL;
        __ada_task_context_dispose(&call->workerContext);
        freeEntryCall(call);
    }
    free(operation);
    return selected;
}

static int matchingAlternative(AdaTaskState* task, AdaRendezvous* call)
{
    for (int i = 0; i < task->choiceCount; ++i) {
        const AdaSelectChoice* choice = &task->choices[i];
        if (choice->open && choice->kind == ADA_SELECT_ACCEPT
            && choice->entry == call->entry && choice->index == call->index) { return i; }
    }
    return -1;
}

static void commitRequest(AdaTaskState* task, AdaRendezvous* call, int alternative)
{
    __ada_entry_selected_locked(call);
    call->alternative = alternative;
    task->selected = call;
    task->choices = NULL;
    task->choiceCount = 0;
    task->waitingTerminate = 0;
    __ada_condition_broadcast(&task->entryChanged);
}

static void unlinkRequest(AdaTaskState* task, AdaRendezvous* call)
{
    AdaRendezvous** slot = &task->calls;
    while (*slot != NULL && *slot != call) { slot = &(*slot)->next; }
    if (*slot != NULL) { *slot = call->next; call->next = NULL; }
}

static void releaseTaskCallLocked(AdaEntryCall* call)
{
    if (call->kind == ADA_ENTRY_TASK && call->registered) {
        AdaTaskState* task = call->task;
        --task->activeCalls;
        call->registered = 0;
        __ada_condition_broadcast(&task->entryChanged);
    }
}

void __ada_entry_finish_locked(AdaEntryCall* call, const AdaException* exception)
{
    releaseTaskCallLocked(call);
    if (exception != NULL) { call->exception.identity = exception; }
    call->done = 1;
    __ada_condition_broadcast(&call->changed);
}

void __ada_entry_selected_locked(AdaEntryCall* call)
{
    call->selected = 1;
    __ada_condition_broadcast(&call->changed);
}

int __ada_entry_cancel_locked(AdaEntryCall* call)
{
    if (call->done || call->selected || !call->abortable) { return 0; }
    if (call->kind == ADA_ENTRY_TASK && call->registered) { unlinkRequest(call->task, call); }
    call->cancelled = 1;
    __ada_entry_finish_locked(call, NULL);
    return 1;
}

void __ada_entry_cancel_request(AdaEntryCall* call)
{
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    call->cancelRequested = 1;
    __ada_condition_broadcast(&call->changed);
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
}

static AdaEntryCall* newEntryCall(AdaTaskContext* context, void* arguments,
    int64_t deadline, enum AdaSelectClock clock, enum AdaEntryCallMode mode)
{
    if (context->protectedActions != NULL) { __ada_raise(ADA_PROGRAM_ERROR); return NULL; }
    AdaEntryCall* call = calloc(1, sizeof *call);
    if (call == NULL) { __ada_raise(ADA_STORAGE_ERROR); return NULL; }
    if (__ada_condition_init(&call->changed) != 0) {
        free(call);
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    call->arguments = arguments;
    call->caller = __ada_current_task();
    call->priority = activePriority(call->caller, context);
    call->abortable = 1;
    call->deadline = deadline;
    call->clock = clock;
    call->mode = mode;
    __ada_occurrence_initialize(&call->exception);
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    AdaTaskState* caller = call->caller;
    call->callerTask = caller;
    call->callerNext = caller->outgoing;
    caller->outgoing = call;
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    return call;
}

static void freeEntryCall(AdaEntryCall* call)
{
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    AdaTaskState* caller = call->callerTask;
    AdaEntryCall** slot = &caller->outgoing;
    while (*slot != NULL && *slot != call) { slot = &(*slot)->callerNext; }
    if (*slot == call) { *slot = call->callerNext; }
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    if (call->redirect != NULL) {
        __ada_protected_target_release(call->redirect->protectedLock, call->redirect->protectedLock != NULL);
        free(call->redirect->arguments);
        free(call->redirect);
    }
    __ada_protected_target_release(call->protectedLock, call->protectedPending);
    __ada_occurrence_release(&call->exception);
    __ada_condition_destroy(&call->changed);
    free(call->ownedArguments);
    free(call);
}

static void enqueueTaskCallLocked(AdaEntryCall* call)
{
    AdaTaskState* task = call->task;
    if (task == NULL) { __ada_entry_finish_locked(call, ADA_PROGRAM_ERROR); return; }
    if (atomic_load(&task->completed) || atomic_load(&task->abortRequested)) {
        __ada_entry_finish_locked(call, ADA_TASKING_ERROR);
        return;
    }
    call->registered = 1;
    ++task->activeCalls;
    int alternative = matchingAlternative(task, call);
    if (alternative >= 0) { commitRequest(task, call, alternative); }
    else {
        AdaEntryCall** tail = &task->calls;
        while (*tail != NULL) { tail = &(*tail)->next; }
        *tail = call;
        __ada_condition_broadcast(&task->entryChanged);
    }
}

static void entryBlockLocked(AdaTaskContext* context, AdaEntryCall* call)
{
    if (context->taskSelf != NULL && !context->helperContext && context->abortDeferralDepth == 0) {
        context->taskSelf->blocked = 1;
        context->taskSelf->blockedCall = call;
    }
}

static void entryUnblockLocked(AdaTaskContext* context)
{
    if (context->taskSelf != NULL && !context->helperContext) {
        context->taskSelf->blocked = 0;
        context->taskSelf->blockedCall = NULL;
        __ada_condition_broadcast(&__ada_main_context.sharedMasterChanged);
    }
}

static int waitEntryCall(AdaTaskContext* context, AdaEntryCall* call)
{
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    while (!call->published) { __ada_condition_wait(&call->changed, &__ada_main_context.sharedMasterMutex); }
    while (!call->done) {
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        __ada_async_poll(context);
        __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
        if (context->exception != NULL) {
            call->cancelRequested = 1;
            // A selected call owns live formal objects. Remember cancellation
            // for a later requeue-with-abort, but leave delivery until safe.
            if (context->exception == &__ada_exc_async_transfer) {
                context->exception = NULL;
                context->asyncTarget = NULL;
            }
        }
        if (call->done) { break; }
        if (call->kind == ADA_ENTRY_PROTECTED && !call->registered) {
            if (call->cancelRequested && call->abortable) {
                __ada_entry_cancel_locked(call);
                continue;
            }
            entryUnblockLocked(context);
            __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
            __ada_protected_request_enqueue(context, call, 0);
            __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
            continue;
        }
        int cancel = call->cancelRequested || call->mode == ADA_ENTRY_CONDITIONAL
            || (call->mode == ADA_ENTRY_TIMED && expired(call->deadline, call->clock));
        if (cancel && call->abortable && !call->selected) {
            if (call->kind == ADA_ENTRY_PROTECTED) {
                entryUnblockLocked(context);
                __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
                __ada_protected_request_cancel(call);
                __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
            } else { __ada_entry_cancel_locked(call); }
            continue;
        }
        if (call->kind == ADA_ENTRY_PROTECTED && call->selected && !call->executing) {
            call->executing = 1;
            entryUnblockLocked(context);
            __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
            __ada_protected_request_execute(context, call);
            __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
            continue;
        }
        entryBlockLocked(context, call);
        int64_t until = __ada_real_clock() + INT64_C(10000000);
        if (call->mode == ADA_ENTRY_TIMED && call->abortable && !call->selected) {
            int64_t deadline = __ada_select_wait_deadline(call->deadline, call->clock);
            if (until > deadline) { until = deadline; }
        }
        __ada_condition_wait_until(&call->changed, &__ada_main_context.sharedMasterMutex, until);
    }
    entryUnblockLocked(context);
    int selected = !call->cancelled;
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    if (call->cancelled) { __ada_async_poll(context); }
    if (context->exception == NULL && call->exception.identity != NULL) { __ada_reraise(&call->exception); }
    return selected;
}

int __ada_task_entry_call_until(AdaTaskContext* context, void* identity, int entry,
    int64_t index, void* arguments, int64_t deadline, enum AdaSelectClock clock,
    enum AdaEntryCallMode mode)
{
    AdaEntryCall* call = newEntryCall(context, arguments, deadline, clock, mode);
    if (call == NULL) { return 0; }
    call->kind = ADA_ENTRY_TASK;
    call->task = identity;
    call->entry = entry;
    call->index = index;
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    enqueueTaskCallLocked(call);
    call->published = 1;
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    int selected = waitEntryCall(context, call);
    freeEntryCall(call);
    return selected;
}

int __ada_protected_entry_call(AdaTaskContext* context, AdaConcurrentObject* object,
    AdaProtectedBarrier barrier, void* link, void (*body)(void*), void* arguments,
    int64_t deadline, enum AdaSelectClock clock, enum AdaEntryCallMode mode)
{
    AdaEntryCall* call = newEntryCall(context, arguments, deadline, clock, mode);
    if (call == NULL) { return 0; }
    call->kind = ADA_ENTRY_PROTECTED;
    call->object = object;
    call->protectedLock = __ada_protected_target_retain(object);
    call->protectedPending = call->protectedLock != NULL;
    if (call->protectedLock == NULL) { freeEntryCall(call); return 0; }
    call->barrier = barrier;
    call->link = link;
    call->body = body;
    __ada_protected_request_enqueue(context, call, 0);
    call->published = 1;
    int selected = waitEntryCall(context, call);
    freeEntryCall(call);
    return selected;
}

static void* runAsyncCall(void* argument)
{
    AdaEntryCall* call = argument;
    __ada_task_context_bind(&call->workerContext);
    waitEntryCall(&call->workerContext, call);
    return NULL;
}

static void* startAsyncCall(AdaTaskContext* context, AdaEntryCall* call)
{
    AdaAsyncSelect* operation = calloc(1, sizeof *operation);
    if (operation == NULL) { freeEntryCall(call); __ada_raise(ADA_STORAGE_ERROR); return NULL; }
    call->workerContext.helperContext = 1;
    call->workerContext.taskSelf = context->taskSelf;
    call->workerContext.currentMaster = context->currentMaster;
    call->workerContext.accessibilityParentLevel = context->accessibilityParentLevel;
    if (__ada_thread_create(&call->worker, runAsyncCall, call) != 0) {
        free(operation);
        freeEntryCall(call);
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    if (call->kind == ADA_ENTRY_PROTECTED) { __ada_protected_request_enqueue(context, call, 0); }
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    if (call->kind == ADA_ENTRY_TASK) { enqueueTaskCallLocked(call); }
    call->published = 1;
    operation->immediate = call->selected || call->done;
    __ada_condition_broadcast(&call->changed);
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    if (context->exception != NULL) {
        __ada_thread_join(call->worker, NULL);
        call->workerContext.taskSelf = NULL;
        call->workerContext.currentMaster = NULL;
        __ada_task_context_dispose(&call->workerContext);
        freeEntryCall(call);
        free(operation);
        return NULL;
    }
    operation->kind = kAsyncTask;
    operation->request = call;
    operation->previous = context->asyncSelects;
    context->asyncSelects = operation;
    return operation;
}

static int copyEntryPacket(AdaEntryCall* call, int64_t bytes)
{
    if (bytes < 0) { return 1; }
    call->ownedArguments = malloc(bytes == 0 ? 1 : (size_t)bytes);
    if (call->ownedArguments == NULL) { __ada_raise(ADA_STORAGE_ERROR); return 0; }
    if (bytes != 0) { memcpy(call->ownedArguments, call->arguments, (size_t)bytes); }
    call->arguments = call->ownedArguments;
    return 1;
}

void* __ada_async_start_task(AdaTaskContext* context, void* identity, int entry, int64_t index, void* arguments)
{
    return __ada_async_start_task_packet(context, identity, entry, index, arguments, -1);
}

void* __ada_async_start_task_packet(AdaTaskContext* context, void* identity, int entry, int64_t index,
    void* arguments, int64_t bytes)
{
    if (context->abortDeferralDepth != 0) { __ada_raise(ADA_PROGRAM_ERROR); return NULL; }
    AdaEntryCall* call = newEntryCall(context, arguments, 0, ADA_SELECT_MONOTONIC, ADA_ENTRY_WAIT);
    if (call == NULL) { return NULL; }
    if (!copyEntryPacket(call, bytes)) { freeEntryCall(call); return NULL; }
    call->kind = ADA_ENTRY_TASK;
    call->task = identity;
    call->entry = entry;
    call->index = index;
    return startAsyncCall(context, call);
}

void* __ada_async_start_protected(AdaTaskContext* context, AdaConcurrentObject* object,
    int (*barrier)(void*, void*), void* link, void (*body)(void*), void* arguments)
{
    return __ada_async_start_protected_packet(context, object, barrier, link, body, arguments, -1);
}

void* __ada_async_start_protected_packet(AdaTaskContext* context, AdaConcurrentObject* object,
    int (*barrier)(void*, void*), void* link, void (*body)(void*), void* arguments, int64_t bytes)
{
    if (context->abortDeferralDepth != 0) { __ada_raise(ADA_PROGRAM_ERROR); return NULL; }
    AdaEntryCall* call = newEntryCall(context, arguments, 0, ADA_SELECT_MONOTONIC, ADA_ENTRY_WAIT);
    if (call == NULL) { return NULL; }
    if (!copyEntryPacket(call, bytes)) { freeEntryCall(call); return NULL; }
    call->kind = ADA_ENTRY_PROTECTED;
    call->object = object;
    call->protectedLock = __ada_protected_target_retain(object);
    call->protectedPending = call->protectedLock != NULL;
    if (call->protectedLock == NULL) { freeEntryCall(call); return NULL; }
    call->barrier = barrier;
    call->link = link;
    call->body = body;
    return startAsyncCall(context, call);
}

static AdaEntryRedirect* prepareRedirect(AdaTaskContext* context, const void* arguments,
    int64_t bytes, int withAbort)
{
    if (context->currentEntry == NULL || context->currentEntry->redirect != NULL || bytes < 0) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return NULL;
    }
    AdaEntryRedirect* redirect = calloc(1, sizeof *redirect);
    if (redirect == NULL) { __ada_raise(ADA_STORAGE_ERROR); return NULL; }
    redirect->arguments = malloc(bytes == 0 ? 1 : (size_t)bytes);
    if (redirect->arguments == NULL) { free(redirect); __ada_raise(ADA_STORAGE_ERROR); return NULL; }
    if (bytes != 0) { memcpy(redirect->arguments, arguments, (size_t)bytes); }
    redirect->withAbort = withAbort;
    context->currentEntry->redirect = redirect;
    return redirect;
}

void __ada_requeue_task(AdaTaskContext* context, void* identity, int entry, int64_t index,
    const void* arguments, int64_t bytes, int withAbort)
{
    AdaEntryRedirect* redirect = prepareRedirect(context, arguments, bytes, withAbort);
    if (redirect == NULL) { return; }
    redirect->kind = ADA_ENTRY_TASK;
    redirect->task = identity;
    redirect->entry = entry;
    redirect->index = index;
}

void __ada_requeue_protected(AdaTaskContext* context, AdaConcurrentObject* object,
    AdaProtectedBarrier barrier, void* link, void (*body)(void*), const void* arguments,
    int64_t bytes, int withAbort, int internal)
{
    AdaEntryRedirect* redirect = prepareRedirect(context, arguments, bytes, withAbort);
    if (redirect == NULL) { return; }
    redirect->kind = ADA_ENTRY_PROTECTED;
    redirect->object = object;
    redirect->protectedLock = __ada_protected_target_retain(object);
    if (redirect->protectedLock == NULL) {
        context->currentEntry->redirect = NULL;
        free(redirect->arguments);
        free(redirect);
        return;
    }
    redirect->barrier = barrier;
    redirect->link = link;
    redirect->body = body;
    redirect->internal = internal;
}

void __ada_entry_complete(AdaTaskContext* context, AdaEntryCall* call, int internal)
{
    AdaEntryRedirect* redirect = call->redirect;
    if (context->exception != NULL && redirect != NULL) {
        __ada_protected_target_release(redirect->protectedLock, redirect->protectedLock != NULL);
        free(redirect->arguments);
        free(redirect);
        redirect = NULL;
        call->redirect = NULL;
    }
    if (context->exception != NULL) {
        const AdaException* identity = context->exception == &__ada_exc_async_transfer
            ? ADA_TASKING_ERROR : context->exception;
        __ada_occurrence_restore(&call->exception, identity, context->pendingMessage, context->pendingMessageLength);
        if (call->exception.identity == NULL) { call->exception.identity = ADA_STORAGE_ERROR; }
        call->exception.origin = context->pendingOrigin;
        call->exception.traceCount = context->pendingTraceCount;
        memcpy(call->exception.trace, context->pendingTrace, (size_t)context->pendingTraceCount * sizeof *context->pendingTrace);
    }
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    if (redirect == NULL) {
        __ada_entry_finish_locked(call, NULL);
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        return;
    }
    releaseTaskCallLocked(call);
    call->next = NULL;
    call->selected = 0;
    call->executing = 0;
    call->registered = 0;
    call->abortable = redirect->withAbort;
    call->requeuedWithAbort |= redirect->withAbort;
    free(call->ownedArguments);
    call->ownedArguments = redirect->arguments;
    call->arguments = redirect->arguments;
    call->kind = redirect->kind;
    call->task = redirect->task;
    call->entry = redirect->entry;
    call->index = redirect->index;
    __ada_protected_target_release_locked(call->protectedLock, call->protectedPending);
    call->object = redirect->object;
    call->protectedLock = redirect->protectedLock;
    call->protectedPending = call->protectedLock != NULL;
    call->barrier = redirect->barrier;
    call->link = redirect->link;
    call->body = redirect->body;
    int sameObject = internal && redirect->internal;
    call->redirect = NULL;
    free(redirect);
    if (call->cancelRequested && call->abortable) { __ada_entry_cancel_locked(call); }
    else if (call->kind == ADA_ENTRY_TASK) { enqueueTaskCallLocked(call); }
    __ada_condition_broadcast(&call->changed);
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    if (sameObject && !call->done) { __ada_protected_request_enqueue(context, call, 1); }
}

void __ada_task_entry_call(AdaTaskContext* context, void* identity, int entry,
                           int64_t index, void* arguments)
{
    __ada_task_entry_call_until(context, identity, entry, index, arguments, 0,
        ADA_SELECT_MONOTONIC, ADA_ENTRY_WAIT);
}

int __ada_task_select(AdaTaskContext* context, const AdaSelectChoice* choices,
    int count, int hasElse, void** request)
{
    AdaTaskState* task = context->taskSelf;
    if (task == NULL || context->protectedActions != NULL || count <= 0) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return -1;
    }
    *request = NULL;
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    task->blocked = 1;
    task->selected = NULL;
    task->terminateSelected = 0;
    task->choices = choices;
    task->choiceCount = count;
    int result = -1;
    for (;;) {
        for (AdaRendezvous* call = task->calls; call != NULL; call = call->next) {
            int alternative = matchingAlternative(task, call);
            if (alternative >= 0) {
                unlinkRequest(task, call);
                commitRequest(task, call, alternative);
                break;
            }
        }
        if (task->selected != NULL) {
            AdaRendezvous* call = task->selected;
            task->selected = NULL;
            call->previousAccept = task->accepted;
            task->accepted = call;
            call->previousContextEntry = context->currentEntry;
            context->currentEntry = call;
            result = call->alternative;
            *request = call;
            break;
        }
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        __ada_async_poll(context);
        __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
        if (task->selected != NULL) {
            if (context->exception == &__ada_exc_async_transfer) { context->exception = NULL; }
            continue;
        }
        if (context->exception != NULL) { break; }
        int open = 0, terminate = -1;
        int64_t next = INT64_MAX;
        for (int i = 0; i < count; ++i) {
            const AdaSelectChoice* choice = &choices[i];
            if (!choice->open) { continue; }
            ++open;
            if (choice->kind == ADA_SELECT_TERMINATE) { terminate = i; }
            if (choice->kind == ADA_SELECT_DELAY) {
                if (expired(choice->deadline, choice->clock)) { result = i; break; }
                int64_t until = __ada_select_wait_deadline(choice->deadline, choice->clock);
                if (until < next) { next = until; }
            }
        }
        if (result >= 0 || hasElse) { break; }
        if (open == 0 || context->exception != NULL) {
            if (context->exception == NULL) { __ada_raise(ADA_PROGRAM_ERROR); }
            break;
        }
        task->waitingTerminate = terminate >= 0;
        checkTerminateCohorts();
        if (task->terminateSelected) { result = terminate; break; }
        int64_t poll = __ada_real_clock() + 10000000;
        if (next > poll) { next = poll; }
        __ada_condition_wait_until(&task->entryChanged, &__ada_main_context.sharedMasterMutex, next);
    }
    task->choices = NULL;
    task->choiceCount = 0;
    task->waitingTerminate = 0;
    task->blocked = 0;
    __ada_condition_broadcast(&__ada_main_context.sharedMasterChanged);
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    if (*request != NULL) {
        AdaEntryCall* call = *request;
        call->savedPriority = __ada_priority_inherit(call->priority);
        if (context->exception != NULL) {
            __ada_task_entry_complete(context, call);
            *request = NULL;
            result = -1;
        }
    }
    return result;
}

void* __ada_task_entry_accept(AdaTaskContext* context, int entry, int64_t index)
{
    AdaSelectChoice choice = {ADA_SELECT_ACCEPT, entry, index, 0, ADA_SELECT_MONOTONIC, 1};
    void* request = NULL;
    __ada_task_select(context, &choice, 1, 0, &request);
    return request;
}

void* __ada_task_entry_arguments(void* request)
{
    return ((AdaRendezvous*)request)->arguments;
}

void __ada_task_entry_complete(AdaTaskContext* context, void* request)
{
    AdaTaskState* task = context->taskSelf;
    AdaEntryCall* call = request;
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    task->accepted = call->previousAccept;
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    context->currentEntry = call->previousContextEntry;
    __ada_priority_leave(call->savedPriority);
    __ada_entry_complete(context, call, 0);
}

void* __ada_task_entry_caller(AdaTaskContext* context)
{
    AdaTaskState* task = context->taskSelf;
    if (task == NULL || task->accepted == NULL) { __ada_raise(ADA_PROGRAM_ERROR); return NULL; }
    return task->accepted->caller;
}

int __ada_task_entry_count(void* identity, int entry, int64_t index)
{
    AdaTaskState* task = identity;
    if (task == NULL) { __ada_raise(ADA_PROGRAM_ERROR); return 0; }
    int count = 0;
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    for (AdaRendezvous* call = task->calls; call != NULL; call = call->next) {
        if (call->entry == entry && call->index == index) { ++count; }
    }
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    return count;
}

void __ada_environment_task_state(int completed, int terminated)
{
    if (!completed && !terminated) {
        __ada_mutex_lock(&environmentTask.mutex);
        environmentTask.priorityThread = __ada_priority_thread_self();
        environmentTask.priorityReady = 1;
        atomic_store(&environmentTask.basePriority, kAdaDefaultPriority);
        __ada_mutex_unlock(&environmentTask.mutex);
    }
    atomic_store(&environmentTask.completed, completed);
    atomic_store(&environmentTask.terminated, terminated);
    if (terminated) { __ada_task_attributes_terminated(&environmentTask); }
}

/* List and scheduling ownership is protected by the environment context's
   master mutex. Never hold it while starting/joining tasks or calling Ada. */
static AdaMasterState* findMaster(AdaMaster* master)
{
    for (AdaMasterState* state = __ada_main_context.sharedMasters; state != NULL;
         state = state->registeredNext) {
        if (state->master == master) {
            return state;
        }
    }
    return NULL;
}

/* Includes enclosing lexical masters and the task executing each master. */
static int taskDependsOn(AdaTaskState* task, AdaMaster* master)
{
    while (task != NULL) {
        for (AdaMaster* ancestor = task->master; ancestor != NULL; ancestor = ancestor->state) {
            if (ancestor == master) { return 1; }
        }
        AdaMasterState* state = findMaster(task->master);
        task = state != NULL ? state->ownerTask : NULL;
    }
    return 0;
}

static void requestAbortLocked(AdaTaskState* task)
{
    if (atomic_load(&task->completed)) { return; }
    atomic_store(&task->abortRequested, 1);
    for (AdaEntryCall* call = task->outgoing; call != NULL; call = call->callerNext) {
        call->cancelRequested = 1;
        __ada_condition_broadcast(&call->changed);
    }
    for (AdaRendezvous* call = task->calls; call != NULL;) {
        AdaRendezvous* next = call->next;
        call->next = NULL;
        __ada_entry_finish_locked(call, ADA_TASKING_ERROR);
        call = next;
    }
    task->calls = NULL;
    __ada_condition_broadcast(&task->entryChanged);
}

static int taskDescendsFrom(AdaTaskState* task, AdaTaskState* parent)
{
    while (task != NULL) {
        if (task == parent) { return 1; }
        AdaMasterState* owner = findMaster(task->master);
        if (owner != NULL && owner->ownerTask == NULL && parent == &environmentTask) { return 1; }
        task = owner != NULL ? owner->ownerTask : NULL;
    }
    return 0;
}

void __ada_task_block_begin(AdaTaskContext* context)
{
    if (context->taskSelf == NULL || context->helperContext || context->abortDeferralDepth != 0) { return; }
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    context->taskSelf->blocked = 1;
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
}

void __ada_task_block_end(AdaTaskContext* context)
{
    if (context->taskSelf == NULL || context->helperContext) { return; }
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    context->taskSelf->blocked = 0;
    context->taskSelf->blockedCall = NULL;
    __ada_condition_broadcast(&__ada_main_context.sharedMasterChanged);
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
}

void __ada_abort_tasks(AdaTaskContext* context, void** identities, int count)
{
    if (context->protectedActions != NULL) { __ada_raise(ADA_PROGRAM_ERROR); return; }
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    for (int i = 0; i < count; ++i) {
        AdaTaskState* root = identities[i];
        if (root == NULL) { continue; }
        if (atomic_load(&root->completed)) { continue; }
        requestAbortLocked(root);
        for (AdaMasterState* state = __ada_main_context.sharedMasters; state != NULL; state = state->registeredNext) {
            for (AdaTaskState* task = state->tasks; task != NULL; task = task->next) {
                if (taskDescendsFrom(task, root)) { requestAbortLocked(task); }
            }
        }
    }
    // A source abort waits for cancellation of a blocked, cancellable
    // operation, but never for a running or abort-deferred action to finish.
    for (int i = 0; i < count; ++i) {
        AdaTaskState* task = identities[i];
        if (task == NULL || task == context->taskSelf) { continue; }
        while (task->blocked && !atomic_load(&task->terminated)
            && !(task->blockedCall != NULL && (task->blockedCall->selected || !task->blockedCall->abortable))) {
            __ada_condition_wait(&__ada_main_context.sharedMasterChanged, &__ada_main_context.sharedMasterMutex);
        }
    }
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
}

void __ada_abort_task(void* identity)
{
    if (identity == NULL) { __ada_raise(ADA_PROGRAM_ERROR); return; }
    __ada_abort_tasks(__ada_task_context(), &identity, 1);
    __ada_async_poll(__ada_task_context());
}

void __ada_master_abort(AdaTaskContext* context, AdaMaster* master)
{
    (void)context;
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    for (AdaMasterState* state = __ada_main_context.sharedMasters; state != NULL; state = state->registeredNext) {
        for (AdaTaskState* task = state->tasks; task != NULL; task = task->next) {
            if (taskDependsOn(task, master) && !atomic_load(&task->terminated)) {
                requestAbortLocked(task);
            }
        }
    }
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
}

static void checkTerminateMaster(AdaMaster* master)
{
    int ready = 1, waiting = 0;
    for (AdaMasterState* state = __ada_main_context.sharedMasters; state != NULL && ready; state = state->registeredNext) {
        for (AdaTaskState* task = state->tasks; task != NULL; task = task->next) {
            if (!taskDependsOn(task, master) || atomic_load(&task->terminated)) { continue; }
            if (!task->waitingTerminate) { ready = 0; break; }
            waiting = 1;
        }
    }
    if (!ready || !waiting) { return; }
    for (AdaMasterState* state = __ada_main_context.sharedMasters; state != NULL; state = state->registeredNext) {
        for (AdaTaskState* task = state->tasks; task != NULL; task = task->next) {
            if (task->waitingTerminate && taskDependsOn(task, master)) {
                task->terminateSelected = 1;
                completeTaskLocked(task);
            }
        }
    }
}

static void checkTerminateCohorts(void)
{
    for (AdaMasterState* state = __ada_main_context.sharedMasters; state != NULL; state = state->registeredNext) {
        for (AdaMaster* master = state->master; master != NULL; master = master->state) {
            if (masterActivities(master) & kMasterCompleting) { checkTerminateMaster(master); }
        }
    }
}

static AdaMasterState* registerMaster(AdaTaskContext* context, AdaMaster* master, int collection)
{
    AdaMasterState* state = calloc(1, sizeof *state);
    if (state == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    state->master = master;
    state->owner = context;
    state->ownerTask = context->taskSelf;
    state->parent = master->state;
    state->collection = collection;
    state->next = context->taskMasters;
    context->taskMasters = state;
    state->registeredNext = __ada_main_context.sharedMasters;
    __ada_main_context.sharedMasters = state;
    atomic_fetch_or_explicit(&master->activities, kMasterRegistered, memory_order_release);
    return state;
}

typedef struct AdaAccessLevel
{
    struct AdaAccessLevel* next;
    AdaMaster* master;
    int id;
    int parent;
} AdaAccessLevel;

/* Called with the shared master mutex held. IDs are never reused, and records
   are removed when their master leaves. No generated stack address escapes. */
static int accessLevelForMaster(AdaMaster* master)
{
    if (master == NULL || master == &__ada_main_context.libraryMaster) { return 0; }
    for (AdaAccessLevel* entry = __ada_main_context.sharedAccessLevels; entry != NULL; entry = entry->next) {
        if (entry->master == master) { return entry->id; }
    }
    int parent = master->state != NULL ? accessLevelForMaster(master->state)
        : __ada_task_context()->accessibilityParentLevel;
    if (__ada_exception != NULL) { return 0; }
    if (__ada_main_context.nextAccessLevel == INT_MAX) { __ada_raise(ADA_STORAGE_ERROR); return 0; }
    AdaAccessLevel* entry = malloc(sizeof *entry);
    if (entry == NULL) { __ada_raise(ADA_STORAGE_ERROR); return 0; }
    *entry = (AdaAccessLevel) {__ada_main_context.sharedAccessLevels, master,
        (int)++__ada_main_context.nextAccessLevel, parent};
    __ada_main_context.sharedAccessLevels = entry;
    atomic_fetch_or_explicit(&master->activities, kMasterAccessLevel, memory_order_release);
    return entry->id;
}

int __ada_current_access_level(void)
{
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    int level = accessLevelForMaster(__ada_task_context()->currentMaster);
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    return level;
}

void __ada_accessibility_check(int sourceLevel, int targetLevel)
{
    if (sourceLevel == 0) { return; }
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    int level = targetLevel;
    while (level != 0 && level != sourceLevel) {
        AdaAccessLevel* entry = __ada_main_context.sharedAccessLevels;
        while (entry != NULL && entry->id != level) { entry = entry->next; }
        level = entry == NULL ? 0 : entry->parent;
    }
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    if (level != sourceLevel) { __ada_raise(ADA_PROGRAM_ERROR); }
}

/* Called with the shared master mutex held. */
static void forgetAccessMaster(AdaMaster* master)
{
    AdaAccessLevel** link = &__ada_main_context.sharedAccessLevels;
    while (*link != NULL && (*link)->master != master) { link = &(*link)->next; }
    if (*link != NULL) {
        AdaAccessLevel* entry = *link;
        *link = entry->next;
        free(entry);
    }
}

void __ada_master_enter(AdaTaskContext* context, AdaMaster* master, int kind)
{
    master->state = context->currentMaster;
    atomic_init(&master->activities, 0);
    master->deferralDepth = context->abortDeferralDepth;
    if (kind == ADA_MASTER_COLLECTION) {
        // Register the access type's owner before foreign tasks can allocate.
        __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
        registerMaster(context, master, 1);
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    } else {
        // Statically empty lexical masters remain allocation-free.
        context->currentMaster = master;
    }
}

void __ada_task_activated(AdaTaskContext* context)
{
    AdaTaskState* task = context->taskSelf;
    if (task == NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    __ada_mutex_lock(&task->mutex);
    atomic_store(&context->inheritedPriority, 0);
    __ada_mutex_unlock(&task->mutex);
    int status = refreshPriority(context);
    __ada_mutex_lock(&task->mutex);
    if (status != 0) { task->activationFailed = 1; }
    task->activated = 1;
    __ada_condition_broadcast(&task->condition);
    __ada_mutex_unlock(&task->mutex);
}

void __ada_task_activation_failed(AdaTaskContext* context)
{
    AdaTaskState* task = context->taskSelf;
    if (task == NULL) {
        return;
    }
    __ada_mutex_lock(&task->mutex);
    if (!task->activated) {
        atomic_store(&task->completed, 1);
        task->activationFailed = !atomic_load(&task->abortRequested);
        task->activated = 1;
        __ada_condition_broadcast(&task->condition);
    }
    __ada_mutex_unlock(&task->mutex);
}

static void* executeTask(void* argument)
{
    AdaTaskState* task = argument;
    task->context.taskSelf = task;
    __ada_task_context_bind(&task->context);
    __ada_mutex_lock(&task->mutex);
    task->priorityThread = __ada_priority_thread_self();
    task->priorityReady = 1;
    atomic_store(&task->context.inheritedPriority, task->activationPriority + 1);
    int priority = activePriority(task, &task->context);
    int status = __ada_thread_set_priority(task->priorityThread, priority);
    task->context.appliedPriority = priority;
    task->context.priorityApplied = status == 0;
    __ada_mutex_unlock(&task->mutex);
    if (status != 0) { __ada_raise(ADA_TASKING_ERROR); }
    __ada_async_poll(&task->context);
    if (task->context.exception == NULL) { task->body(task->link); }
    __ada_task_complete(&task->context);
    __ada_task_activation_failed(&task->context);
    // An exception escaping the handled sequence completes only this task.
    // Generated cleanup has already awaited its dependents and finalized locals.
    __ada_mutex_lock(&task->mutex);
    atomic_store(&task->terminated, 1);
    __ada_mutex_unlock(&task->mutex);
    __ada_task_attributes_terminated(task);
    task->context.taskSelf = NULL;
    __ada_task_context_dispose(&task->context);
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    atomic_store(&task->terminated, 1);
    task->blocked = 0;
    __ada_condition_broadcast(&task->entryChanged);
    checkTerminateCohorts();
    __ada_condition_broadcast(&__ada_main_context.sharedMasterChanged);
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    return NULL;
}

void __ada_task_create_configured(AdaTaskContext* context, AdaConcurrentObject* object, void (*body)(void*),
                            void* link, const int* elaborated, void* collection, void* group,
                            AdaMaster* owner, int priority, int specified, int64_t storageSize)
{
    if (context->protectedActions != NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    if (specified && (priority < kAdaPriorityFirst || priority > kAdaPriorityLast)) {
        __ada_raise(ADA_CONSTRAINT_ERROR); return;
    }
    int64_t actualSize;
    if (__ada_thread_stack_size(storageSize, &actualSize) != 0) {
        __ada_raise(ADA_STORAGE_ERROR); return;
    }
    // AdaCollection begins with its independent master identity.
    AdaMaster* master = owner != NULL ? owner : __ada_task_owner(context, collection);
    if (master == NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    if (master->deferralDepth != 0) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    AdaTaskState* task = calloc(1, sizeof *task);
    if (task == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
        return;
    }
    if (__ada_mutex_init(&task->mutex) != 0) {
        free(task);
        __ada_raise(ADA_STORAGE_ERROR);
        return;
    }
    if (__ada_condition_init(&task->condition) != 0) {
        __ada_mutex_destroy(&task->mutex);
        free(task);
        __ada_raise(ADA_STORAGE_ERROR);
        return;
    }
    if (__ada_condition_init(&task->entryChanged) != 0) {
        __ada_condition_destroy(&task->condition);
        __ada_mutex_destroy(&task->mutex);
        free(task);
        __ada_raise(ADA_STORAGE_ERROR);
        return;
    }
    atomic_init(&task->basePriority, specified ? priority : atomic_load(&((AdaTaskState*)__ada_current_task())->basePriority));
    task->storageSize = actualSize;
    task->stackSpecified = storageSize > 0;
    atomic_init(&task->abortRequested, 0);
    atomic_init(&task->completed, 0);
    atomic_init(&task->terminated, 0);
    snprintf(task->image, sizeof task->image, "task@%p", (void*)task);
    task->object = object;
    task->body = body;
    task->link = link;
    task->group = group;
    task->elaborated = elaborated;
    task->context.accessibilityParentLevel = collection != NULL
        ? __ada_collection_access_level(collection) : owner == NULL ? __ada_current_access_level() : 0;
    if (context->exception != NULL) { __ada_condition_destroy(&task->entryChanged); __ada_condition_destroy(&task->condition); __ada_mutex_destroy(&task->mutex); free(task); return; }
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    AdaMasterState* state = findMaster(master);
    if (state == NULL) {
        state = registerMaster(context, master, collection != NULL);
    }
    if (state != NULL && owner != NULL && collection == NULL) {
        task->context.accessibilityParentLevel = accessLevelForMaster(master);
    }
    if (state == NULL || context->exception != NULL) {
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        __ada_condition_destroy(&task->entryChanged);
        __ada_condition_destroy(&task->condition);
        __ada_mutex_destroy(&task->mutex);
        free(task);
        return;
    }
    task->master = master;
    task->next = state->tasks;
    markDependents(master);
    state->tasks = task;
    object->resource = task;
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
}

void __ada_task_create_owned(AdaTaskContext* context, AdaConcurrentObject* object, void (*body)(void*),
                            void* link, const int* elaborated, void* collection, void* group,
                            AdaMaster* owner)
{
    __ada_task_create_configured(context, object, body, link, elaborated, collection, group,
        owner, kAdaDefaultPriority, 0, 0);
}

AdaMaster* __ada_task_owner(AdaTaskContext* context, void* collection)
{
    return collection != NULL ? collection : context->currentMaster;
}

void __ada_task_create(AdaTaskContext* context, AdaConcurrentObject* object, void (*body)(void*),
                      void* link, const int* elaborated, void* collection, void* group)
{
    __ada_task_create_owned(context, object, body, link, elaborated, collection, group, NULL);
}

void __ada_task_result_await(void* master)
{
    __ada_master_await(__ada_task_context(), master);
}

void* __ada_task_result_mark(AdaMaster* master)
{
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    AdaMasterState* state = findMaster(master);
    void* mark = state != NULL ? state->tasks : NULL;
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    return mark;
}

static void activateGroup(AdaTaskContext* context, AdaMaster* master, void* group)
{
    if (!(masterActivities(master) & kMasterDependents)) {
        return;
    }
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    AdaMasterState* state = findMaster(master);
    if (state == NULL) {
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        return;
    }
    size_t count = 0;
    for (AdaTaskState* task = state->tasks; task != NULL; task = task->next) {
        if (!task->activating && !task->joining && !task->started && !task->joined
            && (group == NULL || group == task->group)) {
            // RM 3.11: check the whole group before starting any member.
            if (!*task->elaborated) {
                __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
                __ada_raise(ADA_PROGRAM_ERROR);
                return;
            }
            ++count;
        }
    }
    if (count == 0) {
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        return;
    }
    AdaTaskState** batch = malloc(count * sizeof *batch);
    if (batch == NULL) {
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        __ada_raise(ADA_STORAGE_ERROR);
        return;
    }
    size_t index = 0;
    for (AdaTaskState* task = state->tasks; task != NULL; task = task->next) {
        if (!task->activating && !task->joining && !task->started && !task->joined
            && (group == NULL || group == task->group)) {
            task->activating = 1;
            batch[index++] = task;
        }
    }
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    int failed = 0;
    int storageFailed = 0;
    for (size_t i = 0; i < count; ++i) {
        AdaTaskState* task = batch[i];
        task->activationPriority = activePriority(__ada_current_task(), context);
        if (__ada_thread_create_stack(&task->thread, executeTask, task, task->storageSize) == 0) {
            // The claim keeps await/discard from inspecting scheduling fields
            // until the complete activation transaction has been published.
            task->started = 1;
        } else {
            completeTask(task);
            atomic_store(&task->terminated, 1);
            __ada_task_attributes_terminated(task);
            task->joined = 1;
            storageFailed |= task->stackSpecified;
            failed = 1;
        }
    }
    for (size_t i = 0; i < count; ++i) {
        AdaTaskState* task = batch[i];
        if (task->started) {
            __ada_mutex_lock(&task->mutex);
            while (!task->activated) {
                __ada_condition_wait(&task->condition, &task->mutex);
            }
            failed |= task->activationFailed && !atomic_load(&task->abortRequested);
            __ada_mutex_unlock(&task->mutex);
        }
    }
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    for (size_t i = 0; i < count; ++i) {
        batch[i]->activating = 0;
    }
    __ada_condition_broadcast(&__ada_main_context.sharedMasterChanged);
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    free(batch);
    if (failed) {
        __ada_raise(storageFailed ? ADA_STORAGE_ERROR : ADA_TASKING_ERROR);
    }
}

void __ada_master_activate(AdaTaskContext* context, AdaMaster* master)
{
    __ada_async_poll(context);
    if (context->exception != NULL) { return; }
    activateGroup(context, master, NULL);
    __ada_async_poll(context);
}

void __ada_task_activate_group(AdaTaskContext* context, void* collection, void* group)
{
    __ada_async_poll(context);
    if (context->exception != NULL) { return; }
    activateGroup(context, collection != NULL ? collection : context->currentMaster, group);
    __ada_async_poll(context);
}

static void awaitTask(AdaTaskState* task)
{
    if (task->started && !task->joined) {
        __ada_thread_join(task->thread, NULL);
    }
    // An unactivated task terminates without running after declaration failure.
    completeTask(task);
    atomic_store(&task->terminated, 1);
    __ada_task_attributes_terminated(task);
}

static int dependsOn(AdaMasterState* state, AdaMaster* master)
{
    if (state->master == master) {
        return 1;
    }
    while (state->collection) {
        if (state->parent == master) {
            return 1;
        }
        state = findMaster(state->parent);
        if (state == NULL) {
            break;
        }
    }
    return 0;
}

/* Await one task at a time, resampling after every join. A running dependent
   may allocate another task in any collection of this lexical master. A fixed
   snapshot or a one-pass collection-first traversal would miss that task. */
enum AdaTaskAwaitMode
{
    kAwaitDependents,
    kDiscardObject,
    kDiscardResult
};

static void awaitTasks(AdaMaster* master, void* object, long size, enum AdaTaskAwaitMode mode,
                       void* mark, void* group)
{
    if (!(masterActivities(master) & kMasterDependents)) {
        return;
    }
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    for (;;) {
        AdaTaskState* selected = NULL;
        AdaTaskState* unactivated = NULL;
        int busy = 0;
        for (AdaMasterState* state = __ada_main_context.sharedMasters; state != NULL;
             state = state->registeredNext) {
            if (mode != kAwaitDependents ? state->master != master : !dependsOn(state, master)) {
                continue;
            }
            for (AdaTaskState* task = state->tasks; task != NULL && task != mark; task = task->next) {
                if (mode == kDiscardResult && task->group != group) { continue; }
                uintptr_t offset = (uintptr_t)task->object - (uintptr_t)object;
                if (mode == kDiscardObject && ((uintptr_t)task->object < (uintptr_t)object || offset >= (uintptr_t)size)) {
                    continue;
                }
                // Do not inspect scheduling fields owned by another activation.
                if (task->activating || task->joining) {
                    busy = 1;
                } else if (!task->joined) {
                    if (task->started) {
                        selected = task;
                        break;
                    }
                    unactivated = task;
                }
            }
            if (selected != NULL) {
                break;
            }
        }
        if (selected == NULL) {
            if (busy) {
                __ada_condition_wait(&__ada_main_context.sharedMasterChanged,
                    &__ada_main_context.sharedMasterMutex);
                continue;
            }
            if (unactivated == NULL) {
                break;
            }
            selected = unactivated;
        }
        selected->joining = 1;
        while (selected->started && !atomic_load(&selected->terminated)) {
            int64_t now = __ada_real_clock();
            __ada_condition_wait_until(&selected->entryChanged, &__ada_main_context.sharedMasterMutex,
                now > INT64_MAX - INT64_C(10000000) ? INT64_MAX : now + INT64_C(10000000));
            __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
            __ada_async_poll(__ada_task_context());
            if (__ada_async_ready(__ada_task_context())) { __ada_master_abort(__ada_task_context(), master); }
            __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
        }
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        awaitTask(selected);
        __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
        selected->joined = 1;
        selected->joining = 0;
        __ada_condition_broadcast(&__ada_main_context.sharedMasterChanged);
    }
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
}

void __ada_task_result_discard(AdaMaster* master, void* mark, void* group)
{
    // A failed construction must cancel dormant tasks before its result storage
    // and protected/controlled components are released. Records remain owned by
    // the master so Task_Id queries keep their existing lifetime contract.
    awaitTasks(master, NULL, 0, kDiscardResult, mark, group);
}

void __ada_master_await(AdaTaskContext* context, AdaMaster* master)
{
    (void)context;
    if (masterActivities(master) & kMasterDependents) {
        __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
        atomic_fetch_or_explicit(&master->activities, kMasterCompleting, memory_order_release);
        checkTerminateCohorts();
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    }
    awaitTasks(master, NULL, 0, kAwaitDependents, NULL, NULL);
}

void __ada_task_discard(AdaTaskContext* context, AdaMaster* master, void* object, long size)
{
    (void)context;
    awaitTasks(master, object, size, kDiscardObject, NULL, NULL);
}

void __ada_master_leave(AdaTaskContext* context, AdaMaster* master)
{
    __ada_master_await(context, master);
    unsigned int activities = masterActivities(master);
    AdaMasterState* state = NULL;
    if (activities & (kMasterRegistered | kMasterAccessLevel)) {
        __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
        state = activities & kMasterRegistered ? findMaster(master) : NULL;
        if (state != NULL) {
            AdaMasterState** entry = &state->owner->taskMasters;
            while (*entry != state) {
                entry = &(*entry)->next;
            }
            *entry = state->next;
            entry = &__ada_main_context.sharedMasters;
            while (*entry != state) {
                entry = &(*entry)->registeredNext;
            }
            *entry = state->registeredNext;
        }
        if (activities & kMasterAccessLevel) {
            forgetAccessMaster(master);
        }
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    }
    if (state != NULL) {
        while (state->tasks != NULL) {
            AdaTaskState* task = state->tasks;
            state->tasks = task->next;
            // Established callers must finish waking before their target's
            // condition and mutex can be destroyed, including foreign callers
            // that are not dependents of this Ada master.
            __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
            while (task->activeCalls != 0) {
                __ada_condition_wait(&task->entryChanged, &__ada_main_context.sharedMasterMutex);
            }
            __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
            __ada_condition_destroy(&task->entryChanged);
            __ada_condition_destroy(&task->condition);
            __ada_mutex_destroy(&task->mutex);
            free(task);
        }
        free(state);
    }
    if (context->currentMaster == master) {
        context->currentMaster = master->state;
    }
    master->state = NULL;
    atomic_store_explicit(&master->activities, 0, memory_order_relaxed);
}

void __ada_access_collection_parent(AdaMaster* master, int level)
{
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    AdaMaster* parent = level == 0 ? &__ada_main_context.libraryMaster : NULL;
    for (AdaAccessLevel* entry = __ada_main_context.sharedAccessLevels; parent == NULL && entry != NULL; entry = entry->next) {
        if (entry->id == level) { parent = entry->master; }
    }
    AdaMasterState* state = findMaster(master);
    if (state != NULL && parent != NULL) {
        state->parent = parent;
        master->state = parent;
        if (masterActivities(master) & kMasterDependents) {
            markDependents(parent);
        }
    }
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    if (parent == NULL || state == NULL) { __ada_raise(ADA_PROGRAM_ERROR); }
}
