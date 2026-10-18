/* Test replacement for adamaster.c: observe real generated/runtime hook
   boundaries without adding test callbacks or mutable globals to the runtime. */
#include "../runtime/adart.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "master hook check failed at line %d\n", __LINE__); abort(); \
} } while (0)

typedef struct MasterState
{
    AdaMaster* master;
    AdaTaskContext* context;
    struct MasterState* previous;
    struct MasterState* next;
    int kind;
    int activated;
    int awaited;
} MasterState;

static _Thread_local MasterState* top;
static _Thread_local MasterState* records;
static _Thread_local MasterState* owners[32];
static _Thread_local int completed[32];
static _Thread_local int skippedActivation;
static _Thread_local int blocks;

void __ada_master_enter(AdaTaskContext* context, AdaMaster* master, int kind)
{
    CHECK(context == __ada_task_context());
    for (MasterState* live = records; live != NULL; live = live->next) {
        CHECK(live->master != master);
    }
    MasterState* state = calloc(1, sizeof *state);
    CHECK(state != NULL);
    *state = (MasterState) { master, context, top, records, kind, 0, 0 };
    master->state = state;
    records = state;
    if (kind != ADA_MASTER_COLLECTION) {
        top = state;
    }
    blocks += kind == ADA_MASTER_BLOCK;
}

void __ada_master_activate(AdaTaskContext* context, AdaMaster* master)
{
    MasterState* state = master->state;
    CHECK(state != NULL && state->context == context && !state->awaited);
    CHECK(state->kind == ADA_MASTER_COLLECTION || (state == top && !state->activated));
    state->activated = 1;
    /* A future activation failure must leave through the same master cleanup
       as declaration failure, before executing the block's statements. */
    if (owners[18] == state) {
        __ada_raise(ADA_TASKING_ERROR);
    }
}

void __ada_master_await(AdaTaskContext* context, AdaMaster* master)
{
    MasterState* state = master->state;
    CHECK(state != NULL && state->context == context && !state->awaited);
    CHECK(state->kind == ADA_MASTER_COLLECTION || state == top);
    skippedActivation += !state->activated;
    state->awaited = 1;
}

void __ada_master_leave(AdaTaskContext* context, AdaMaster* master)
{
    MasterState* state = master->state;
    CHECK(state != NULL && state->context == context && state->awaited);
    for (int i = 0; i < 32; ++i) {
        CHECK(owners[i] != state);
    }
    int library = state->kind == ADA_MASTER_LIBRARY;
#ifdef EXPECT_LIBRARY_FAILURE
    if (library) {
        CHECK(!state->activated);
    }
#endif
    if (state->kind != ADA_MASTER_COLLECTION) {
        CHECK(top == state);
        top = state->previous;
    }
    MasterState** link = &records;
    while (*link != state) {
        CHECK(*link != NULL);
        link = &(*link)->next;
    }
    *link = state->next;
    free(state);
    master->state = NULL;
    if (library) {
        CHECK(records == NULL && top == NULL && completed[1] && skippedActivation > 0);
#ifndef EXPECT_FAILURE
        CHECK(blocks >= 10);
        for (int i = 1; i <= 20; ++i) {
            CHECK(completed[i]);
        }
#endif
        puts("master hooks ok");
    }
}

int rememberMaster(int id)
{
    CHECK(id > 0 && id < 32 && owners[id] == NULL && top != NULL);
    owners[id] = top;
    return id;
}

int rememberCollection(int id)
{
    CHECK(id > 0 && id < 32 && owners[id] == NULL);
    MasterState* state = records;
    while (state != NULL && state->kind != ADA_MASTER_COLLECTION) {
        state = state->next;
    }
    CHECK(state != NULL && state->activated);
    owners[id] = state;
    return id;
}

void masterFinalized(int id)
{
    CHECK(id > 0 && id < 32 && owners[id] != NULL);
    CHECK(owners[id]->awaited);
    owners[id] = NULL;
    completed[id] = 1;
    if (id == 12) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
    }
}

void masterCheckDone(int id)
{
    CHECK(id > 0 && id < 32 && completed[id]);
}

int masterFail(void)
{
    __ada_raise_message(ADA_CONSTRAINT_ERROR, "elaboration failure", 19);
    return 0;
}
