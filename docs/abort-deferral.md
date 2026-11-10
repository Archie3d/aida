# Abort deferral and explicit emitter effects

Phase 6a foundation, targeting [Ada 2005 RM 9.8](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-8.html)
and [AARM 9.8](https://www.adaic.org/resources/add_content/standards/05aarm/html/AA-9-8.html).
Phase 9 adds [asynchronous select](select-statements.md) and
[source abort/requeue cancellation](requeue-abort.md) on these balanced regions.
Preemptive abort and full abort conformance are not claimed.

## Task-owned regions

`AdaTaskContext.abortDeferralDepth` starts at zero. Only the task using that
context changes it. `__ada_abort_defer` increments it and
`__ada_abort_undefer` decrements it; nesting composes across Ada/C callbacks.
Wrong-context operations, overflow and underflow are runtime invariant failures.
Disposal refuses a context with an outstanding region. The pending exception
remains at ABI offset zero; generated code never reads the counter directly.
The helpers do not allocate, inspect/clear an exception, or deliver abort.

Protected resource acquisition begins a region after validation; successful
leave/complete ends it after releasing the mutex and completing entry servicing.
An internal operation reuses its enclosing action. A rejected external self-call
adds no region. Different-object nesting balances independently. A queued entry
caller leaves its region before waiting and re-enters only if selected; failed
barriers and failed queued callers leave no region behind. Barrier evaluation
runs under the servicing task's region. Waiting for a selected servicing chain
keeps the original action deferred until its completion.

`__ada_finalize_to` brackets the whole chain, including nested callbacks,
exception suspension/restoration and Program_Error substitution on failure.
This covers lexical cleanup, temporary/result failure, library shutdown,
collections, deallocation and controlled assignment finalizers. The separate
ancestor-view finalization callback is bracketed too. Inactive records are never
finalized again. An ordinary explicit Ada call to Finalize does not itself add a
region, as required by AARM 9.8(12.b).

## Explicit calls and exits

`emitCallInstruction` takes a `CallEffects` value. `CheckException` publishes the
current AST source location and immediately uses the shared exception check.
`DeferredCheck` publishes that location but leaves the check to its caller, for
example after releasing a protected action or joining indirect-call branches.
`RuntimeHook` and `CheckedRuntimeHook` preserve the triggering location for
master/arena bookkeeping, with deferred or immediate checking respectively.
These hooks can still raise; preserving a location does not imply no effects.
The ordinary `line` writer no longer recognizes calls or returns from strings.

`emitReturn` emits lexical master exits and records an explicit activation-exit
insertion point. `finishFunction` applies one ordered exit-action list at those
points after discovering all activation resources, including allocations in a
branch emitted after an early return. Task completion precedes master await;
then temporary and local finalization precede arena release, master leave and
trace leave. Propagation uses the same return helper. Jumps to handlers,
propagation or loop exits share `emitBlockExits` with returns; conditional edges
that cross masters route through it. Handler and loop storage checkpoints retain
their existing cleanup and normal-return-only copy-back rules.

The extraction preserves the existing ABI and emitted IR: 83 units from twelve
programs covering masters, controlled results, protected actions, entry failures
and task activation were compared byte for byte before adding the runtime regions.

## Phase 9 delivery contract

An abort request lives in synchronized task state, separate from the pending
Ada exception. Another task does not mutate the deferral counter or inject an
ordinary catchable exception into the target's context. The completion-point
hook checks the request and current depth. Nonzero depth retains the request
without initiating transfer. Decrementing to zero only permits delivery; it
does not jump out of a C helper while it restores an occurrence, releases a
resource or finishes its caller's transaction.

Completion-point polls cover activation initiation/completion, entry and delay
start/end, abort statements and task termination (RM 9.8(15–18)). Explicit
emitter call/check and scope-exit helpers handle the resulting abort edge.
Propagation bypasses Ada handlers, awaits dependent tasks, finalizes remaining
objects exactly once and releases storage inside-out.
Ordinary exception checks remain operational inside deferred regions.

Polling alone cannot stop blocked operations. The Phase 9 queue/cancellation
protocol wakes blocked delays, protected-entry waiters and rendezvous, attempts
entry cancellation, and defers while an uncancellable call finishes.
No asynchronous host-thread cancellation, signals or longjmp through Ada/C frames.
Phase 9 now brackets implicit Initialize/Adjust callbacks and the entire
assignment operation with controlled parts, including failure exits. Deferred
master ownership is recorded in the existing master record: task creation for a
master entered inside such a region raises Program_Error. Asynchronous select
has its own transfer target and nesting identity. These mechanisms are tested
by `selectdeferral` and the [select lifecycle controls](select-statements.md).
Source abort now uses this delivery protocol; [its contract](requeue-abort.md)
records activation, queue cancellation, protected-action and partition behavior.
Preemptive delivery remains outside the supported surface.

## Evidence and remaining coverage

`runtime.abort_deferral` runs nested/failing chain and view callbacks on four
threads, checking exactly-once finalization, pending-message preservation and
balanced depth. `runtime.protected_lifecycle` and `runtime.entry_lifecycle` check
nested actions, barrier/service execution, queue failures and successful release.
`abortdeferral.adb` executes protected failure cleanup, exception unwinding and
access deallocation from four Ada tasks in whole and separate compilation;
`determinism.abortdeferral` covers the corresponding IR. Existing rejection tests
continue to reject unsupported tasking syntax.

At the Phase 6a foundation milestone, the selected ACATS suite was unchanged:
no abort behavior became executable in that foundation. Those unchanged
selected cases were not reported as passing abort coverage. The current Phase 9 evidence is recorded separately in the
[requeue/abort contract](requeue-abort.md).

Historical Phase 6a validation on this host: all 672 CTest cases passed, including the 17 selected
ACATS expectations. ASan/UBSan passes for the four C runtime lifecycle/deferral
tests and both compilation modes of the Ada deferral regression. QBE-generated
Ada memory accesses are not sanitizer-instrumented.
