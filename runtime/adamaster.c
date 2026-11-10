#include "adamaster.h"
#include "adart.h"
#include "adathread.h"
#include "adatime.h"
#include "adacalendar.h"
#include "adaprotected.h"
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

typedef struct AdaRendezvous
{
    struct AdaRendezvous* next;
    struct AdaRendezvous* previousAccept;
    void* caller;
    void* arguments;
    int entry;
    int64_t index;
    int done;
    int selected;
    int alternative;
    AdaExceptionOccurrence exception;
} AdaRendezvous;

typedef struct AdaTaskState
{
    struct AdaTaskState* next;
    void* object;
    void (*body)(void*);
    void* link;
    void* group;
    const int* elaborated;
    AdaThread thread;
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

static AdaTaskState environmentTask = { .image = "environment" };

void* __ada_current_task(void)
{
    AdaTaskContext* context = __ada_task_context();
    return context->taskSelf != NULL ? context->taskSelf : &environmentTask;
}

void* __ada_task_identity(void** object)
{
    if (*object == NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
    }
    return *object;
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
    return !atomic_load(&((AdaTaskState*)identity)->completed);
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
    for (AdaRendezvous* call = task->calls; call != NULL; call = call->next) {
        call->exception.identity = ADA_TASKING_ERROR;
        call->done = 1;
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

int __ada_async_run(void* operation)
{
    return !((AdaAsyncSelect*)operation)->immediate;
}

static int asyncCompleted(AdaAsyncSelect* operation)
{
    if (operation->kind == kAsyncDelay) { return expired(operation->deadline, operation->clock); }
    if (operation->kind == kAsyncProtected) { return __ada_protected_async_done(operation->request); }
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    int done = ((AdaRendezvous*)operation->request)->done;
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    return done;
}

int __ada_async_ready(AdaTaskContext* context)
{
    if (context->taskSelf != NULL && context->abortDeferralDepth == 0
        && atomic_load(&context->taskSelf->abortRequested)) { return 1; }
    for (AdaAsyncSelect* operation = context->asyncSelects; operation != NULL; operation = operation->previous) {
        if (context->abortDeferralDepth <= operation->deferralDepth && asyncCompleted(operation)) { return 1; }
    }
    return 0;
}

void __ada_async_poll(AdaTaskContext* context)
{
    if (context->exception != NULL) { return; }
    if (context->taskSelf != NULL && context->abortDeferralDepth == 0
        && atomic_load(&context->taskSelf->abortRequested)) {
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
    if (operation->kind == kAsyncTask) {
        AdaTaskState* server = operation->server;
        AdaRendezvous* call = operation->request;
        __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
        if (!call->selected && !call->done) {
            AdaRendezvous** slot = &server->calls;
            while (*slot != NULL && *slot != call) { slot = &(*slot)->next; }
            if (*slot != NULL) { *slot = call->next; }
            selected = 0;
        } else {
            selected = 1;
            while (!call->done) { __ada_condition_wait(&server->entryChanged, &__ada_main_context.sharedMasterMutex); }
        }
        --server->activeCalls;
        __ada_condition_broadcast(&server->entryChanged);
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        if (context->exception == NULL && call->exception.identity != NULL) { __ada_reraise(&call->exception); }
        __ada_occurrence_release(&call->exception);
        free(call);
    } else if (operation->kind == kAsyncProtected) {
        selected = __ada_protected_async_finish(context, operation->request);
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
    call->selected = 1;
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

static AdaRendezvous* startTaskRequest(AdaTaskContext* context, void* identity, int entry,
    int64_t index, void* arguments)
{
    if (identity == NULL || context->protectedActions != NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return NULL;
    }
    AdaTaskState* task = identity;
    AdaRendezvous* call = calloc(1, sizeof *call);
    if (call == NULL) { __ada_raise(ADA_STORAGE_ERROR); return NULL; }
    call->entry = entry;
    call->index = index;
    call->caller = __ada_current_task();
    call->arguments = arguments;
    __ada_occurrence_initialize(&call->exception);
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    if (atomic_load(&task->completed)) {
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        free(call);
        __ada_raise(ADA_TASKING_ERROR);
        return NULL;
    }
    ++task->activeCalls;
    int alternative = matchingAlternative(task, call);
    if (alternative >= 0) {
        commitRequest(task, call, alternative);
    } else {
        AdaRendezvous** tail = &task->calls;
        while (*tail != NULL) { tail = &(*tail)->next; }
        *tail = call;
    }
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    return call;
}

void* __ada_async_start_task(AdaTaskContext* context, void* identity, int entry, int64_t index, void* arguments)
{
    if (context->abortDeferralDepth != 0) { __ada_raise(ADA_PROGRAM_ERROR); return NULL; }
    AdaAsyncSelect* operation = calloc(1, sizeof *operation);
    if (operation == NULL) { __ada_raise(ADA_STORAGE_ERROR); return NULL; }
    AdaRendezvous* call = startTaskRequest(context, identity, entry, index, arguments);
    if (call == NULL) { free(operation); return NULL; }
    operation->kind = kAsyncTask;
    operation->server = identity;
    operation->request = call;
    operation->deferralDepth = context->abortDeferralDepth;
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    operation->immediate = call->selected || call->done;
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    operation->previous = context->asyncSelects;
    context->asyncSelects = operation;
    return operation;
}

void* __ada_async_start_protected(AdaTaskContext* context, void** object,
    int (*barrier)(void*, void*), void* link, void (*body)(void*), void* arguments)
{
    if (context->abortDeferralDepth != 0) { __ada_raise(ADA_PROGRAM_ERROR); return NULL; }
    AdaAsyncSelect* operation = calloc(1, sizeof *operation);
    if (operation == NULL) { __ada_raise(ADA_STORAGE_ERROR); return NULL; }
    operation->request = __ada_protected_async_start(context, object, barrier, link, body, arguments, &operation->immediate);
    if (operation->request == NULL) { free(operation); return NULL; }
    operation->kind = kAsyncProtected;
    operation->deferralDepth = context->abortDeferralDepth;
    operation->previous = context->asyncSelects;
    context->asyncSelects = operation;
    return operation;
}

int __ada_task_entry_call_until(AdaTaskContext* context, void* identity, int entry,
    int64_t index, void* arguments, int64_t deadline, enum AdaSelectClock clock,
    enum AdaEntryCallMode mode)
{
    AdaRendezvous* call = startTaskRequest(context, identity, entry, index, arguments);
    if (call == NULL) { return 0; }
    AdaTaskState* task = identity;
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    int success = 1;
    while (!call->done) {
        if (!call->selected) {
            __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
            __ada_async_poll(context);
            __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
            if (!call->selected && context->exception != NULL) {
                unlinkRequest(task, call);
                success = 0;
                break;
            }
            // Commitment wins a cancellation race; finish the rendezvous first.
            if (call->selected && context->exception == &__ada_exc_async_transfer) { context->exception = NULL; }
        }
        if (!call->selected && (mode == ADA_ENTRY_CONDITIONAL
            || (mode == ADA_ENTRY_TIMED && expired(deadline, clock)))) {
            unlinkRequest(task, call);
            success = 0;
            break;
        }
        if (!call->selected && mode == ADA_ENTRY_TIMED) {
            int64_t until = __ada_select_wait_deadline(deadline, clock);
            int64_t poll = __ada_real_clock() + 10000000;
            if (until > poll) { until = poll; }
            __ada_condition_wait_until(&task->entryChanged, &__ada_main_context.sharedMasterMutex, until);
        } else {
            __ada_condition_wait_until(&task->entryChanged, &__ada_main_context.sharedMasterMutex, __ada_real_clock() + 10000000);
        }
    }
    --task->activeCalls;
    __ada_condition_broadcast(&task->entryChanged);
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    if (call->exception.identity != NULL) { __ada_reraise(&call->exception); }
    __ada_occurrence_release(&call->exception);
    free(call);
    return success;
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
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
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
    AdaRendezvous* call = request;
    if (context->exception != NULL) {
        const AdaException* identity = context->exception == &__ada_exc_async_transfer
            ? ADA_TASKING_ERROR : context->exception;
        __ada_occurrence_restore(&call->exception, identity,
            context->pendingMessage, context->pendingMessageLength);
        if (call->exception.identity == NULL) { call->exception.identity = ADA_STORAGE_ERROR; }
        call->exception.origin = context->pendingOrigin;
        call->exception.traceCount = context->pendingTraceCount;
        memcpy(call->exception.trace, context->pendingTrace,
            (size_t)context->pendingTraceCount * sizeof *context->pendingTrace);
    }
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    task->accepted = call->previousAccept;
    call->done = 1;
    __ada_condition_broadcast(&task->entryChanged);
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
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
    atomic_store(&environmentTask.completed, completed);
    atomic_store(&environmentTask.terminated, terminated);
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

void __ada_master_abort(AdaTaskContext* context, AdaMaster* master)
{
    (void)context;
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    for (AdaMasterState* state = __ada_main_context.sharedMasters; state != NULL; state = state->registeredNext) {
        for (AdaTaskState* task = state->tasks; task != NULL; task = task->next) {
            if (taskDependsOn(task, master) && !atomic_load(&task->terminated)) {
                atomic_store(&task->abortRequested, 1);
                __ada_condition_broadcast(&task->entryChanged);
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
        task->activationFailed = 1;
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
    task->body(task->link);
    __ada_task_complete(&task->context);
    __ada_task_activation_failed(&task->context);
    // An exception escaping the handled sequence completes only this task.
    // Generated cleanup has already awaited its dependents and finalized locals.
    task->context.taskSelf = NULL;
    __ada_task_context_dispose(&task->context);
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    atomic_store(&task->terminated, 1);
    __ada_condition_broadcast(&task->entryChanged);
    checkTerminateCohorts();
    __ada_condition_broadcast(&__ada_main_context.sharedMasterChanged);
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    return NULL;
}

void __ada_task_create_owned(AdaTaskContext* context, void** object, void (*body)(void*),
                            void* link, const int* elaborated, void* collection, void* group,
                            AdaMaster* owner)
{
    if (context->protectedActions != NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
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
    *object = task;
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
}

AdaMaster* __ada_task_owner(AdaTaskContext* context, void* collection)
{
    return collection != NULL ? collection : context->currentMaster;
}

void __ada_task_create(AdaTaskContext* context, void** object, void (*body)(void*),
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
    (void)context;
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
    for (size_t i = 0; i < count; ++i) {
        AdaTaskState* task = batch[i];
        if (__ada_thread_create(&task->thread, executeTask, task) == 0) {
            // The claim keeps await/discard from inspecting scheduling fields
            // until the complete activation transaction has been published.
            task->started = 1;
        } else {
            completeTask(task);
            atomic_store(&task->terminated, 1);
            task->joined = 1;
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
            failed |= task->activationFailed;
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
        __ada_raise(ADA_TASKING_ERROR);
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
