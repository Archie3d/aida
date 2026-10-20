#ifndef ADAPROTECTED_H
#define ADAPROTECTED_H

typedef struct AdaTaskContext AdaTaskContext;

typedef struct AdaProtectedAction
{
    void* lock;
    struct AdaProtectedAction* previous;
} AdaProtectedAction;

typedef int (*AdaProtectedBarrier)(void* link, void* object);

/* Entry callers retain their actuals and lexical environment while queued.
   Completion services open queues before releasing the protected action. */
void __ada_protected_entry_enter(AdaTaskContext* context, void** object,
    AdaProtectedAction* action, AdaProtectedBarrier barrier, void* link);
int __ada_protected_count(void** object, AdaProtectedBarrier barrier);
void __ada_protected_complete(AdaTaskContext* context, AdaProtectedAction* action);

void __ada_protected_init(void** object);
void __ada_protected_destroy(void** object);
void __ada_protected_enter(AdaTaskContext* context, void** object, AdaProtectedAction* action);
void __ada_protected_leave(AdaTaskContext* context, AdaProtectedAction* action);

#endif
