# Controlled assignment and adjustment failure

Controlled records, arrays and class-wide objects share the existing per-task
ownership chains and component walks. Assignment evaluates the source, target
and applicable constraint checks before finalizing the old target.

Self-assignment is omitted. For disjoint source and target storage, assignment
finalizes the old target, copies the source and adjusts the actual target,
without an anonymous adjusted copy (Ada 2005 RM 7.6(21)). Both static emission
and dynamic class-wide assignment check storage overlap using unsigned address
distances, without adding sizes to pointers.

Adjustment visits components before their containing controlled object. A user
Adjust exception is recorded while the remaining adjustments run; Program_Error
is raised afterwards (RM 7.6.1(16)). Successfully adjusted parts remain active
in their ownership chain. A failed Adjust leaves its part inactive, choosing
the permitted policy of omitting its finalization. Subsequent assignment and
scope cleanup finalize the successful parts exactly once.

Overlapping views, including array slices, retain an owned adjusted snapshot
before target finalization. A failed snapshot adjustment unwinds its successful
parts and leaves the target unchanged; an unadjusted snapshot is never copied
into storage whose old value might own the source's resources. Successful
snapshots stay owned through target adjustment and statement cleanup.

A target Finalize exception completes the other target finalizations, raises
Program_Error and prevents copying/adjusting the new value. Deallocation also
completes the remaining finalizations before propagating Program_Error.
Initialization failures keep their existing partial-construction cleanup;
successfully initialized or adjusted parts are finalized, failed parts are not.
Whole-assignment abort deferral remains Phase 9 ([foundation](abort-deferral.md)).

`controlledadjustment` covers first/middle/last component failures in records,
arrays and class-wide assignments, containing Adjust failure, recovery,
reference-counted ownership, overlapping slices, initialization, failing
finalization and deallocation. It runs whole/per-unit and determinism checks
and repeats from four tasks with independent local state. Unchanged ACATS
C761006 passes and is promoted into the strict suite. `runtime.array_storage`
checks dynamic target adjustment, no snapshot allocation for disjoint objects,
exception propagation and registration/allocation balance. These runtime paths
and the Ada regression pass with ASan/UBSan on the C runtime; generated QBE
machine code is not sanitizer-instrumented.
