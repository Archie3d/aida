#include "adamaster.h"
#include "adart.h"
#include "adathread.h"

#include <stdlib.h>

_Static_assert(sizeof(AdaMaster) == 8, "master record QBE ABI");

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
    int started;
    int activated;
    int activationFailed;
    int activationObserved;
    int joined;
    AdaTaskContext context;
} AdaTaskState;

typedef struct AdaMasterState
{
    struct AdaMasterState* next;
    AdaMaster* master;
    AdaTaskState* tasks;
    int collection;
} AdaMasterState;

static AdaMasterState* findMaster(AdaTaskContext* context, AdaMaster* master)
{
    for (AdaMasterState* state = context->taskMasters; state != NULL; state = state->next) {
        if (state->master == master) {
            return state;
        }
    }
    return NULL;
}

void __ada_master_enter(AdaTaskContext* context, AdaMaster* master, int kind)
{
    // Empty masters remain allocation-free. The caller-owned identity stores
    // its lexical predecessor; dependent lists are allocated only on creation.
    master->state = context->currentMaster;
    if (kind != ADA_MASTER_COLLECTION) {
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
    __ada_task_activation_failed(&task->context);
    // An exception escaping the handled sequence completes only this task.
    // Generated cleanup has already awaited its dependents and finalized locals.
    task->context.taskSelf = NULL;
    __ada_task_context_dispose(&task->context);
    return NULL;
}

void __ada_task_create(AdaTaskContext* context, void** object, void (*body)(void*),
                      void* link, const int* elaborated, void* collection, void* group)
{
    // AdaCollection begins with its independent master identity.
    AdaMaster* master = collection != NULL ? collection : context->currentMaster;
    if (master == NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    AdaMasterState* state = findMaster(context, master);
    if (state == NULL) {
        state = calloc(1, sizeof *state);
        if (state == NULL) {
            __ada_raise(ADA_STORAGE_ERROR);
            return;
        }
        state->master = master;
        state->collection = collection != NULL;
        state->next = context->taskMasters;
        context->taskMasters = state;
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
    task->object = object;
    task->body = body;
    task->link = link;
    task->group = group;
    task->elaborated = elaborated;
    task->next = state->tasks;
    state->tasks = task;
    *object = task;
}

static void activateGroup(AdaTaskContext* context, AdaMaster* master, void* group)
{
    AdaMasterState* state = findMaster(context, master);
    if (state == NULL) {
        return;
    }
    // RM 3.11: check every body before starting any member of this group.
    for (AdaTaskState* task = state->tasks; task != NULL; task = task->next) {
        if (!task->started && !task->joined && (group == NULL || group == task->group) && !*task->elaborated) {
            __ada_raise(ADA_PROGRAM_ERROR);
            return;
        }
    }
    int failed = 0;
    for (AdaTaskState* task = state->tasks; task != NULL; task = task->next) {
        if (!task->started && !task->joined && (group == NULL || group == task->group)) {
            if (__ada_thread_create(&task->thread, executeTask, task) == 0) {
                task->started = 1;
            } else {
                task->joined = 1;
                failed = 1;
            }
        }
    }
    for (AdaTaskState* task = state->tasks; task != NULL; task = task->next) {
        if (task->started && !task->joined && !task->activationObserved
            && (group == NULL || group == task->group)) {
            __ada_mutex_lock(&task->mutex);
            while (!task->activated) {
                __ada_condition_wait(&task->condition, &task->mutex);
            }
            failed |= task->activationFailed;
            task->activationObserved = 1;
            __ada_mutex_unlock(&task->mutex);
        }
    }
    if (failed) {
        __ada_raise(ADA_TASKING_ERROR);
    }
}

void __ada_master_activate(AdaTaskContext* context, AdaMaster* master)
{
    activateGroup(context, master, NULL);
}

void __ada_task_activate_group(AdaTaskContext* context, void* collection, void* group)
{
    activateGroup(context, collection != NULL ? collection : context->currentMaster, group);
}

static void awaitTask(AdaTaskState* task)
{
    if (task->started && !task->joined) {
        __ada_thread_join(task->thread, NULL);
    }
    // Tasks whose activator never reached activation terminate without running.
    task->joined = 1;
}

void __ada_master_await(AdaTaskContext* context, AdaMaster* master)
{
    // Allocated tasks depend on the master of their access type, even when
    // allocation happened in a shorter-lived subprogram. Join collections
    // before any controlled local of that master can be finalized.
    for (AdaMasterState* child = context->taskMasters; child != NULL; child = child->next) {
        if (child->collection && child->master->state == master) {
            __ada_master_await(context, child->master);
        }
    }
    AdaMasterState* state = findMaster(context, master);
    if (state != NULL) {
        for (AdaTaskState* task = state->tasks; task != NULL; task = task->next) {
            awaitTask(task);
        }
    }
}

void __ada_task_discard(AdaTaskContext* context, AdaMaster* master, void* object, long size)
{
    AdaMasterState* state = findMaster(context, master);
    if (state != NULL) {
        uintptr_t first = (uintptr_t)object;
        for (AdaTaskState* task = state->tasks; task != NULL; task = task->next) {
            uintptr_t address = (uintptr_t)task->object;
            if (address >= first && address - first < (uintptr_t)size) {
                awaitTask(task);
            }
        }
    }
}

void __ada_master_leave(AdaTaskContext* context, AdaMaster* master)
{
    AdaMasterState** entry = &context->taskMasters;
    while (*entry != NULL && (*entry)->master != master) {
        entry = &(*entry)->next;
    }
    if (*entry != NULL) {
        AdaMasterState* state = *entry;
        *entry = state->next;
        while (state->tasks != NULL) {
            AdaTaskState* task = state->tasks;
            state->tasks = task->next;
            awaitTask(task);
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
}
