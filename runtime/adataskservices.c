#include "adataskservices.h"
#include "adart.h"
#include "adathread.h"
#include "adatime.h"

#include <errno.h>
#include <stdlib.h>

/* Registry locks protect membership/references only. Ada assignment/finalizers
   run under the instance lock, never under the registry or master mutex. */
typedef struct AttributeInstance AttributeInstance;
typedef struct AttributeRow
{
    struct AttributeRow* next;
    AttributeInstance* instance;
    void* task;
    void* value;
    AdaFinalization cleanup;
} AttributeRow;

struct AttributeInstance
{
    AttributeInstance* next;
    AdaMutex mutex;
    AttributeRow* rows;
    unsigned int references;
    uint64_t serial;
};

static AdaMutex attributesMutex = ADA_MUTEX_INITIALIZER;
static AdaCondition attributesChanged = ADA_CONDITION_INITIALIZER;
static AttributeInstance* instances;
static uint64_t instanceSerial;

static void releaseInstance(AttributeInstance* instance)
{
    __ada_mutex_lock(&attributesMutex);
    --instance->references;
    __ada_condition_broadcast(&attributesChanged);
    __ada_mutex_unlock(&attributesMutex);
}

static void destroyRows(AttributeRow* rows)
{
    AdaFinalization* cleanup = NULL;
    for (AttributeRow* row = rows; row != NULL; row = row->next) {
        if (row->value != NULL) {
            __ada_finalization_push(&cleanup, &row->cleanup, row->value, __ada_deallocate);
            row->value = NULL;
        }
    }
    __ada_finalize_to(&cleanup, NULL);
    while (rows != NULL) {
        AttributeRow* next = rows->next;
        free(rows);
        rows = next;
    }
}

void* __ada_task_attributes_create(void)
{
    AttributeInstance* instance = calloc(1, sizeof *instance);
    if (instance == NULL) { __ada_raise(ADA_STORAGE_ERROR); return NULL; }
    if (__ada_mutex_init(&instance->mutex) != 0) {
        free(instance);
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    __ada_mutex_lock(&attributesMutex);
    if (instanceSerial == UINT64_MAX) {
        __ada_mutex_unlock(&attributesMutex);
        __ada_mutex_destroy(&instance->mutex);
        free(instance);
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    instance->serial = ++instanceSerial;
    instance->next = instances;
    instances = instance;
    __ada_mutex_unlock(&attributesMutex);
    return instance;
}

void __ada_task_attributes_close(void* key)
{
    if (key == NULL) { return; }
    AdaTaskContext* context = __ada_task_context();
    __ada_abort_defer(context);
    __ada_mutex_lock(&attributesMutex);
    AttributeInstance** slot = &instances;
    while (*slot != NULL && *slot != key) { slot = &(*slot)->next; }
    AttributeInstance* instance = *slot;
    if (instance == NULL) {
        __ada_mutex_unlock(&attributesMutex);
        __ada_raise(ADA_PROGRAM_ERROR);
        __ada_abort_undefer(context);
        return;
    }
    *slot = instance->next;
    while (instance->references != 0) { __ada_condition_wait(&attributesChanged, &attributesMutex); }
    __ada_mutex_unlock(&attributesMutex);
    destroyRows(instance->rows);
    __ada_mutex_destroy(&instance->mutex);
    free(instance);
    __ada_abort_undefer(context);
}

void* __ada_task_attribute_acquire(void* key, void* task)
{
    if (task == NULL) { __ada_raise(ADA_PROGRAM_ERROR); return NULL; }
    if (__ada_task_is_terminated(task)) { __ada_raise(ADA_TASKING_ERROR); return NULL; }
    AdaTaskContext* context = __ada_task_context();
    __ada_abort_defer(context);
    __ada_mutex_lock(&attributesMutex);
    AttributeInstance* instance = instances;
    while (instance != NULL && instance != key) { instance = instance->next; }
    if (instance != NULL) { ++instance->references; }
    __ada_mutex_unlock(&attributesMutex);
    if (instance == NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        __ada_abort_undefer(context);
        return NULL;
    }
    __ada_mutex_lock(&instance->mutex);
    if (__ada_task_is_terminated(task)) {
        __ada_mutex_unlock(&instance->mutex);
        releaseInstance(instance);
        __ada_raise(ADA_TASKING_ERROR);
        __ada_abort_undefer(context);
        return NULL;
    }
    AttributeRow* row = instance->rows;
    while (row != NULL && row->task != task) { row = row->next; }
    if (row == NULL) {
        row = calloc(1, sizeof *row);
        if (row == NULL) {
            __ada_mutex_unlock(&instance->mutex);
            releaseInstance(instance);
            __ada_raise(ADA_STORAGE_ERROR);
            __ada_abort_undefer(context);
            return NULL;
        }
        row->instance = instance;
        row->task = task;
        row->next = instance->rows;
        instance->rows = row;
    }
    return row;
}

void __ada_task_attribute_release(void* token)
{
    if (token == NULL) { return; }
    AttributeRow* row = token;
    AttributeInstance* instance = row->instance;
    __ada_mutex_unlock(&instance->mutex);
    releaseInstance(instance);
    __ada_abort_undefer(__ada_task_context());
}

void* __ada_task_attribute_get(void* token)
{
    return ((AttributeRow*)token)->value;
}

void __ada_task_attribute_put(void* token, void* value)
{
    ((AttributeRow*)token)->value = value;
}

void __ada_task_attributes_terminated(void* task)
{
    /* Serial cursors allow closing/creating other instances in user finalizers
       without keeping stale next pointers or allocating a termination snapshot. */
    __ada_mutex_lock(&attributesMutex);
    uint64_t limit = instanceSerial;
    __ada_mutex_unlock(&attributesMutex);
    uint64_t cursor = 0;
    for (;;) {
        __ada_mutex_lock(&attributesMutex);
        AttributeInstance* instance = NULL;
        for (AttributeInstance* walk = instances; walk != NULL; walk = walk->next) {
            if (walk->serial > cursor && walk->serial <= limit
                && (instance == NULL || walk->serial < instance->serial)) { instance = walk; }
        }
        if (instance != NULL) { ++instance->references; cursor = instance->serial; }
        __ada_mutex_unlock(&attributesMutex);
        if (instance == NULL) { break; }
        __ada_mutex_lock(&instance->mutex);
        AttributeRow** slot = &instance->rows;
        while (*slot != NULL && (*slot)->task != task) { slot = &(*slot)->next; }
        AttributeRow* row = *slot;
        if (row != NULL) { *slot = row->next; row->next = NULL; destroyRows(row); }
        __ada_mutex_unlock(&instance->mutex);
        releaseInstance(instance);
    }
}

typedef struct SuspensionWaiter
{
    int released;
} SuspensionWaiter;

typedef struct SuspensionState
{
    AdaMutex mutex;
    AdaCondition changed;
    SuspensionWaiter* waiter;
    unsigned int references;
    int state;
    int disposed;
} SuspensionState;

static AdaMutex suspensionsMutex = ADA_MUTEX_INITIALIZER;
static AdaCondition suspensionsChanged = ADA_CONDITION_INITIALIZER;

static SuspensionState* suspension(void** slot, int create)
{
    __ada_mutex_lock(&suspensionsMutex);
    SuspensionState* state = *slot;
    if (state == NULL && create) {
        state = calloc(1, sizeof *state);
        if (state != NULL) {
            if (__ada_mutex_init(&state->mutex) != 0) { free(state); state = NULL; }
            else if (__ada_condition_init(&state->changed) != 0) {
                __ada_mutex_destroy(&state->mutex);
                free(state);
                state = NULL;
            }
        }
        *slot = state;
    }
    if (state != NULL) { ++state->references; }
    __ada_mutex_unlock(&suspensionsMutex);
    if (state == NULL && create) { __ada_raise(ADA_STORAGE_ERROR); }
    return state;
}

static void releaseSuspension(SuspensionState* state)
{
    __ada_mutex_lock(&suspensionsMutex);
    --state->references;
    __ada_condition_broadcast(&suspensionsChanged);
    __ada_mutex_unlock(&suspensionsMutex);
}

void __ada_suspension_destroy(void** slot)
{
    __ada_mutex_lock(&suspensionsMutex);
    SuspensionState* state = *slot;
    *slot = NULL;
    __ada_mutex_unlock(&suspensionsMutex);
    if (state == NULL) { return; }
    __ada_mutex_lock(&state->mutex);
    state->disposed = 1;
    __ada_condition_broadcast(&state->changed);
    __ada_mutex_unlock(&state->mutex);
    __ada_mutex_lock(&suspensionsMutex);
    while (state->references != 0) { __ada_condition_wait(&suspensionsChanged, &suspensionsMutex); }
    __ada_mutex_unlock(&suspensionsMutex);
    __ada_condition_destroy(&state->changed);
    __ada_mutex_destroy(&state->mutex);
    free(state);
}

void __ada_suspension_set(void** slot, int value)
{
    SuspensionState* state = suspension(slot, value != 0);
    if (state == NULL) { return; }
    __ada_mutex_lock(&state->mutex);
    if (value && state->waiter != NULL) {
        state->waiter->released = 1;
        state->waiter = NULL;
        state->state = 0;
        __ada_condition_broadcast(&state->changed);
    } else { state->state = value != 0; }
    int disposed = state->disposed;
    __ada_mutex_unlock(&state->mutex);
    releaseSuspension(state);
    if (disposed) { __ada_raise(ADA_PROGRAM_ERROR); }
}

int __ada_suspension_state(void** slot)
{
    SuspensionState* state = suspension(slot, 0);
    if (state == NULL) { return 0; }
    __ada_mutex_lock(&state->mutex);
    int value = state->state, disposed = state->disposed;
    __ada_mutex_unlock(&state->mutex);
    releaseSuspension(state);
    if (disposed) { __ada_raise(ADA_PROGRAM_ERROR); }
    return value;
}

void __ada_suspension_wait(void** slot)
{
    AdaTaskContext* context = __ada_task_context();
    if (context->protectedActions != NULL) { __ada_raise(ADA_PROGRAM_ERROR); return; }
    SuspensionState* state = suspension(slot, 1);
    if (state == NULL) { return; }
    __ada_mutex_lock(&state->mutex);
    if (state->waiter != NULL || state->disposed) {
        __ada_mutex_unlock(&state->mutex);
        releaseSuspension(state);
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    SuspensionWaiter waiter = {0};
    if (state->state) { state->state = 0; waiter.released = 1; }
    else { state->waiter = &waiter; }
    while (!waiter.released && !state->disposed && context->exception == NULL) {
        int status = __ada_condition_wait_until(&state->changed, &state->mutex,
            __ada_real_clock() + INT64_C(10000000));
        __ada_mutex_unlock(&state->mutex);
        if (status != 0 && status != ETIMEDOUT) { __ada_raise(ADA_PROGRAM_ERROR); }
        __ada_async_poll(context);
        __ada_mutex_lock(&state->mutex);
    }
    if (state->waiter == &waiter) { state->waiter = NULL; }
    int disposed = state->disposed;
    __ada_mutex_unlock(&state->mutex);
    releaseSuspension(state);
    if (disposed && context->exception == NULL) { __ada_raise(ADA_PROGRAM_ERROR); }
}
