#ifndef ADAMASTER_H
#define ADAMASTER_H

typedef struct AdaTaskContext AdaTaskContext;

/* Caller-owned identity, live until its dependents have terminated and local
   finalization has finished. Empty lexical masters need no allocation;
   collections register ownership before foreign tasks can use them. */
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

/* Task_Id values remain valid while the corresponding task object exists.
   Queries synchronize with completion and termination without joining tasks. */
void* __ada_current_task(void);
void* __ada_task_identity(void** object);
int __ada_task_id_equal(void* left, void* right);
const char* __ada_task_image(void* identity);
int __ada_task_is_callable(void* identity);
int __ada_task_is_terminated(void* identity);
void __ada_task_complete(AdaTaskContext* context);
void __ada_environment_task_state(int completed, int terminated);

#endif
