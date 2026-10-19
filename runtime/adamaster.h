#ifndef ADAMASTER_H
#define ADAMASTER_H

typedef struct AdaTaskContext AdaTaskContext;

/* Caller-owned identity, live until its dependents have terminated and local
   finalization has finished. Empty masters need no allocation. */
typedef struct AdaMaster
{
    void* state;
} AdaMaster;

enum AdaMasterKind
{
    ADA_MASTER_SUBPROGRAM,
    ADA_MASTER_BLOCK,
    ADA_MASTER_LIBRARY,
    ADA_MASTER_COLLECTION
};

/* Enter before declarations, activate the group after successful elaboration,
   await before finalization/storage release, and leave afterwards. Collection
   masters are independent of the lexical stack and may activate repeatedly.
   Await/leave preserve the activator's pending exception. */
void __ada_master_enter(AdaTaskContext* context, AdaMaster* master, int kind);
void __ada_master_activate(AdaTaskContext* context, AdaMaster* master);
void __ada_master_await(AdaTaskContext* context, AdaMaster* master);
void __ada_master_leave(AdaTaskContext* context, AdaMaster* master);

/* Generated task body ABI, including a lexical link for library bodies. */
void __ada_task_create(AdaTaskContext* context, void** object, void (*body)(void*),
                      void* link, const int* elaborated, void* collection, void* group);
void __ada_task_activate_group(AdaTaskContext* context, void* collection, void* group);
void __ada_task_activated(AdaTaskContext* context);
void __ada_task_activation_failed(AdaTaskContext* context);
void __ada_task_discard(AdaTaskContext* context, AdaMaster* master, void* object, long size);

#endif
