#include "adaprotected.h"
#include "adart.h"
#include "adalock.h"

#include <stdlib.h>

_Static_assert(sizeof(AdaProtectedAction) == 16, "protected action QBE ABI");

typedef struct AdaProtectedLock
{
    AdaMutex mutex;
} AdaProtectedLock;

void __ada_protected_init(void** object)
{
    *object = NULL;
    AdaProtectedLock* lock = malloc(sizeof *lock);
    if (lock == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
        return;
    }
    if (__ada_mutex_init(&lock->mutex) != 0) {
        free(lock);
        __ada_raise(ADA_STORAGE_ERROR);
        return;
    }
    *object = lock;
}

void __ada_protected_destroy(void** object)
{
    AdaProtectedLock* lock = *object;
    if (lock != NULL) {
        if (__ada_mutex_destroy(&lock->mutex) != 0) {
            __ada_raise(ADA_PROGRAM_ERROR);
            return;
        }
        free(lock);
        *object = NULL;
    }
}

void __ada_protected_enter(AdaTaskContext* context, void** object, AdaProtectedAction* action)
{
    AdaProtectedLock* lock = *object;
    if (lock == NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    for (AdaProtectedAction* held = context->protectedActions; held != NULL; held = held->previous) {
        if (held->lock == lock) {
            // An external call on the same object is a potentially blocking
            // operation. Internal calls never acquire another execution resource.
            __ada_raise(ADA_PROGRAM_ERROR);
            return;
        }
    }
    __ada_mutex_lock(&lock->mutex);
    action->lock = lock;
    action->previous = context->protectedActions;
    context->protectedActions = action;
}

void __ada_protected_leave(AdaTaskContext* context, AdaProtectedAction* action)
{
    if (context->protectedActions != action) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    context->protectedActions = action->previous;
    AdaProtectedLock* lock = action->lock;
    __ada_mutex_unlock(&lock->mutex);
}
