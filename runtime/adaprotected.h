#ifndef ADAPROTECTED_H
#define ADAPROTECTED_H

#include "../common/Concurrent.h"

#include <stdint.h>
#include "../common/Select.h"

typedef struct AdaTaskContext AdaTaskContext;

typedef struct AdaProtectedAction
{
    void* lock;
    struct AdaProtectedAction* previous;
    int savedPriority;
} AdaProtectedAction;

typedef int (*AdaProtectedBarrier)(void* link, void* object);

/* Entry callers retain their actuals and lexical environment while queued.
   Completion services open queues before releasing the protected action. */
int __ada_protected_entry_enter_until(AdaTaskContext* context, AdaConcurrentObject* object,
    AdaProtectedAction* action, AdaProtectedBarrier barrier, void* link,
    int64_t deadline, enum AdaSelectClock clock, enum AdaEntryCallMode mode);
void __ada_protected_entry_enter(AdaTaskContext* context, AdaConcurrentObject* object,
    AdaProtectedAction* action, AdaProtectedBarrier barrier, void* link);
int __ada_protected_count(AdaConcurrentObject* object, AdaProtectedBarrier barrier);
void __ada_protected_complete(AdaTaskContext* context, AdaProtectedAction* action);

int __ada_protected_callback_equal(const void* left, const void* right);
void __ada_protected_init(AdaConcurrentObject* object);
void __ada_protected_init_access(AdaConcurrentObject* object, int accessLevel);
int __ada_protected_access_level(AdaConcurrentObject* object);
void __ada_protected_set_ceiling(AdaConcurrentObject* object, int priority);
void __ada_protected_destroy(AdaConcurrentObject* object);
void __ada_protected_enter(AdaTaskContext* context, AdaConcurrentObject* object, AdaProtectedAction* action);
void __ada_protected_leave(AdaTaskContext* context, AdaProtectedAction* action);

#endif
