#ifndef ADAENTRY_H
#define ADAENTRY_H

#include "../common/Concurrent.h"

#include "adart.h"

enum AdaEntryTargetKind
{
    ADA_ENTRY_TASK,
    ADA_ENTRY_PROTECTED
};

/* A call retains its original caller, deadline and formal objects across
   requeues. Mutable state and changed use the shared master mutex. Only the
   runner acquires a protected resource or removes a protected queue node. */
typedef struct AdaEntryRedirect
{
    enum AdaEntryTargetKind kind;
    void* task;
    int entry;
    int64_t index;
    AdaConcurrentObject* object;
    void* protectedLock;
    AdaProtectedBarrier barrier;
    void* link;
    void (*body)(void*);
    void* arguments;
    int withAbort;
    int internal;
} AdaEntryRedirect;

typedef struct AdaEntryCall
{
    struct AdaEntryCall* next;
    struct AdaEntryCall* callerNext;
    void* callerTask;
    struct AdaEntryCall* previousAccept;
    struct AdaEntryCall* previousContextEntry;
    void* caller;
    void* arguments;
    void* ownedArguments;
    int entry;
    int64_t index;
    int done;
    int selected;
    int alternative;
    AdaExceptionOccurrence exception;
    AdaCondition changed;
    enum AdaEntryTargetKind kind;
    void* task;
    AdaConcurrentObject* object;
    void* protectedLock;
    AdaProtectedBarrier barrier;
    void* link;
    void (*body)(void*);
    void* protectedWaiter;
    int protectedPending;
    int registered;
    int published;
    int executing;
    int cancelled;
    int cancelRequested;
    int abortable;
    int requeuedWithAbort;
    int64_t deadline;
    enum AdaSelectClock clock;
    enum AdaEntryCallMode mode;
    AdaEntryRedirect* redirect;
    AdaThread worker;
    AdaTaskContext workerContext;
} AdaEntryCall;

void* __ada_protected_target_retain(AdaConcurrentObject* object);
void __ada_protected_target_release_locked(void* lock, int pending);
void __ada_protected_target_release(void* lock, int pending);
void __ada_entry_finish_locked(AdaEntryCall* call, const AdaException* exception);
void __ada_entry_selected_locked(AdaEntryCall* call);
void __ada_entry_complete(AdaTaskContext* context, AdaEntryCall* call, int internal);
int __ada_entry_cancel_locked(AdaEntryCall* call);
void __ada_entry_cancel_request(AdaEntryCall* call);
void __ada_protected_request_enqueue(AdaTaskContext* context, AdaEntryCall* call, int internal);
void __ada_protected_request_execute(AdaTaskContext* context, AdaEntryCall* call);
int __ada_protected_request_cancel(AdaEntryCall* call);

int __ada_protected_entry_call(AdaTaskContext* context, AdaConcurrentObject* object,
    AdaProtectedBarrier barrier, void* link, void (*body)(void*), void* arguments,
    int64_t deadline, enum AdaSelectClock clock, enum AdaEntryCallMode mode);
void __ada_requeue_task(AdaTaskContext* context, void* identity, int entry, int64_t index,
    const void* arguments, int64_t bytes, int withAbort);
void __ada_requeue_protected(AdaTaskContext* context, AdaConcurrentObject* object,
    AdaProtectedBarrier barrier, void* link, void (*body)(void*), const void* arguments,
    int64_t bytes, int withAbort, int internal);

#endif
