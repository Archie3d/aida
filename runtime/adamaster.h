#ifndef ADAMASTER_H
#define ADAMASTER_H

typedef struct AdaTaskContext AdaTaskContext;

/* Stable, caller-owned identity. Reserved for Phase 4 dependent-task state.
   Stack records live through finalization; library/collection records live
   with their owners. No allocation, scheduling, waiting, or exception changes
   occur in the Phase 2 implementation. */
typedef struct AdaMaster
{
    void* m_state;
} AdaMaster;

enum AdaMasterKind
{
    ADA_MASTER_SUBPROGRAM,
    ADA_MASTER_BLOCK,
    ADA_MASTER_LIBRARY,
    ADA_MASTER_COLLECTION
};

/* Enter before declarations; activate only after successful elaboration.
   Await before any finalization/storage release, leave afterwards. Failed
   elaboration still awaits/leaves, without activation. Context is explicit.
   Collections have independent lifetimes and are not lexical stack entries.
   Await/leave must preserve a pending exception and invoke no Ada callbacks
   under a lock. Activation may report Tasking_Error in Phase 4; generated
   callers already check the context. Collections activate after each
   successful allocator, so their activation hook may run more than once.
   Phase 4 will attach dependents to these identities. */
void __ada_master_enter(AdaTaskContext* context, AdaMaster* master, int kind);
void __ada_master_activate(AdaTaskContext* context, AdaMaster* master);
void __ada_master_await(AdaTaskContext* context, AdaMaster* master);
void __ada_master_leave(AdaTaskContext* context, AdaMaster* master);

#endif
