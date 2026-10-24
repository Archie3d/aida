# Shared access-type collections

Phase 6a implements shared collections for the supported named object access
subset. The lifetime rules follow [Ada 2005 RM 7.6.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-7-6-1.html),
[9.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-3.html)
and [13.11.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-11-2.html).

## Ownership and object operations

Every named object access type now creates a collection, including scalar and
plain composite designations. Derived access types share their ultimate
ancestor's collection. Access-to-subprogram types own no allocation collection.
An allocator records its cleared storage in the access type's collection before
initializing it. Initialization failure follows the same partial-object cleanup
transaction used for controlled allocations. Scope exit reclaims remaining plain
allocations as well as finalizing controlled/protected components.

Any task may allocate, assign to a controlled designated object, or deallocate
through the shared type. A collection belongs to the master that elaborated its
access type; it does not belong to the task that happens to allocate a member.
Allocated tasks also register under that collection's original master. An
allocating task can terminate and dispose its context while the object lives on.

Heap finalization records and allocation records use locked indexes owned by the
environment task context. Stack finalization chains, exception occurrences,
traces and temporary arenas remain local to their executing task. Registration
recognizes allocation-owned chains, including the caller-owned result arena used
by limited/class-wide construction. This avoids retaining pointers into a
terminated allocating task's registry. Per-allocation component chains and arenas
remain with the allocation until it is released.

The storage mutex protects index membership and collection links. Object access
and publication still require Ada synchronization: it does not make simultaneous
mutation or freeing the same object safe. Lookups may return an unpinned record
because the caller must already own/synchronize that object's lifetime. Separate
objects can be initialized, adjusted and finalized concurrently.

Deallocation detaches the allocation under the storage mutex, then waits for its
allocated tasks, finalizes components and releases storage without holding either
service lock. Callbacks execute in the deallocating task's bound context, with
existing exception suspension and abort-deferral rules. Reentrant callbacks can
allocate or deallocate another collection member. `Free(null)` remains a no-op,
including while a limited result has an unpublished null object address.

## Masters, activation and closure

Collection masters register their owning context eagerly, before other tasks
can allocate through them. Empty lexical masters remain allocation-free. The
master index and task-list/scheduling transitions use a separate environment
context mutex. Groups are validated and claimed under that mutex; starting tasks,
waiting for activation, joins and Ada callbacks happen after releasing it.
Activation and joining claims prevent another waiter from treating an unfinished
activation as an unactivated task, or joining the same thread twice. A condition
wakes competing waiters when a claim completes.

Master await resamples dependencies after every join. Running tasks can allocate
new tasks into any collection of the enclosing master, so a fixed snapshot or
one pass over collections is insufficient. Started tasks and outstanding
activation/join transactions are drained before remaining unactivated tasks are
retired. Thus all dependent tasks terminate before ordinary local finalization,
including locals declared after the access type. Collection cleanup awaits its
dependents, marks the collection closed, drains remaining allocations and leaves
its independent master. Allocation into a closing collection raises Program_Error.

Native C hosts must bind one context per worker and join their own native workers
before releasing a collection. Those host threads are outside Ada's master graph.
Library storage creation and library shutdown still belong to the environment
task; other tasks use an already elaborated library collection. Direct use of a
foreign task's stack finalization chain remains unsupported.

The pending exception offset, eight-byte master identity, forty-byte finalization
record and owned-result calling ABI are unchanged. Shared service fields are
appended to the task context; its environment mutexes/condition survive disposal.
Disposal checks shared indexes under their locks before clearing environment
state. Compiler fingerprints invalidate cached units for the new plain-access
collection emission; the hello golden IR records the intentional library setup.

## Evidence and limits

`runtime.shared_collections` runs four allocating workers, then four deallocating
workers after creator-context disposal, with nested callbacks, preserved pending
messages, failing finalizers and limited-result arenas. It also allocates tasks
from foreign contexts and has those tasks create further dependents while the
original master awaits them. Existing allocation-failure instrumentation,
task-context, shared-table and lifecycle tests remain active.

`sharedcollections` covers a library collection passed through protected entries,
controlled assignment and deallocation by consumers, derived access types,
partial-initialization failure and finalization of surviving local allocations.
`sharedtaskcollections` covers concurrent task allocators, deallocation through a
derived type, and a controlled sentinel checking that dependent tasks terminate
before local finalization. Both run whole/separate and determinism checks.

All 680 CTest expectations pass, including 18 selected ACATS expectations.
ASan/UBSan passes six affected C runtime tests; ThreadSanitizer passes five.
Both new Ada programs pass whole/separate modes with each instrumented runtime.
Generated Ada memory accesses are not instrumented, so this verifies runtime
boundaries rather than proving absence of every Ada data race.

Unchanged upstream ACATS C761002 remains a compile XFAIL for the missing root
`Unchecked_Deallocation` compatibility generic (Phase 6e renaming). Labels/goto
are implemented; local `gotocleanup` covers collection and master cleanup on
outward transfers. C761002 is not counted as an executable upstream pass.
General/anonymous access, accessibility extensions, custom storage
pools and runtime-constrained array allocators remain outside this change.
Cross-host validation remains Phase 8 work; abort delivery remains Phase 9.
