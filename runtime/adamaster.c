#include "adamaster.h"
#include "adart.h"
#include "adathread.h"

#include <stdlib.h>
#include <stdatomic.h>
#include <stdio.h>

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
    int activating;
    int joining;
    int activated;
    int activationFailed;
    int joined;
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
} AdaMasterState;

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

void __ada_task_complete(AdaTaskContext* context)
{
    if (context->taskSelf != NULL) {
        atomic_store(&context->taskSelf->completed, 1);
    }
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

static AdaMasterState* registerMaster(AdaTaskContext* context, AdaMaster* master, int collection)
{
    AdaMasterState* state = calloc(1, sizeof *state);
    if (state == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    state->master = master;
    state->owner = context;
    state->parent = master->state;
    state->collection = collection;
    state->next = context->taskMasters;
    context->taskMasters = state;
    state->registeredNext = __ada_main_context.sharedMasters;
    __ada_main_context.sharedMasters = state;
    return state;
}

void __ada_master_enter(AdaTaskContext* context, AdaMaster* master, int kind)
{
    master->state = context->currentMaster;
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
    atomic_store(&task->terminated, 1);
    return NULL;
}

void __ada_task_create(AdaTaskContext* context, void** object, void (*body)(void*),
                      void* link, const int* elaborated, void* collection, void* group)
{
    if (context->protectedActions != NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    // AdaCollection begins with its independent master identity.
    AdaMaster* master = collection != NULL ? collection : context->currentMaster;
    if (master == NULL) {
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
    atomic_init(&task->completed, 0);
    atomic_init(&task->terminated, 0);
    snprintf(task->image, sizeof task->image, "task@%p", (void*)task);
    task->object = object;
    task->body = body;
    task->link = link;
    task->group = group;
    task->elaborated = elaborated;
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    AdaMasterState* state = findMaster(master);
    if (state == NULL) {
        state = registerMaster(context, master, collection != NULL);
    }
    if (state == NULL) {
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        __ada_condition_destroy(&task->condition);
        __ada_mutex_destroy(&task->mutex);
        free(task);
        return;
    }
    task->next = state->tasks;
    state->tasks = task;
    *object = task;
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
}

static void activateGroup(AdaTaskContext* context, AdaMaster* master, void* group)
{
    (void)context;
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
            atomic_store(&task->completed, 1);
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
    // An unactivated task terminates without running after declaration failure.
    atomic_store(&task->completed, 1);
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
static void awaitTasks(AdaMaster* master, void* object, long size, int discard)
{
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    for (;;) {
        AdaTaskState* selected = NULL;
        AdaTaskState* unactivated = NULL;
        int busy = 0;
        for (AdaMasterState* state = __ada_main_context.sharedMasters; state != NULL;
             state = state->registeredNext) {
            if (discard ? state->master != master : !dependsOn(state, master)) {
                continue;
            }
            for (AdaTaskState* task = state->tasks; task != NULL; task = task->next) {
                uintptr_t offset = (uintptr_t)task->object - (uintptr_t)object;
                if (discard && ((uintptr_t)task->object < (uintptr_t)object || offset >= (uintptr_t)size)) {
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
        __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
        awaitTask(selected);
        __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
        selected->joined = 1;
        selected->joining = 0;
        __ada_condition_broadcast(&__ada_main_context.sharedMasterChanged);
    }
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
}

void __ada_master_await(AdaTaskContext* context, AdaMaster* master)
{
    (void)context;
    awaitTasks(master, NULL, 0, 0);
}

void __ada_task_discard(AdaTaskContext* context, AdaMaster* master, void* object, long size)
{
    (void)context;
    awaitTasks(master, object, size, 1);
}

void __ada_master_leave(AdaTaskContext* context, AdaMaster* master)
{
    __ada_master_await(context, master);
    __ada_mutex_lock(&__ada_main_context.sharedMasterMutex);
    AdaMasterState* state = findMaster(master);
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
    __ada_mutex_unlock(&__ada_main_context.sharedMasterMutex);
    if (state != NULL) {
        while (state->tasks != NULL) {
            AdaTaskState* task = state->tasks;
            state->tasks = task->next;
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
