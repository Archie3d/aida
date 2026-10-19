#ifndef ADAPROTECTED_H
#define ADAPROTECTED_H

typedef struct AdaTaskContext AdaTaskContext;

typedef struct AdaProtectedAction
{
    void* lock;
    struct AdaProtectedAction* previous;
} AdaProtectedAction;

void __ada_protected_init(void** object);
void __ada_protected_destroy(void** object);
void __ada_protected_enter(AdaTaskContext* context, void** object, AdaProtectedAction* action);
void __ada_protected_leave(AdaTaskContext* context, AdaProtectedAction* action);

#endif
