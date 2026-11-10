# Requeue and abort

Phase 9 supports `requeue`, `requeue ... with abort`, task-name lists in `abort`,
and `Ada.Task_Identification.Abort_Task` on the existing entry/task surfaces.
This extends the [entry](task-entries.md), [select](select-statements.md) and
[abort-deferral](abort-deferral.md) contracts; it does not establish full tasking
or annex conformance.

## Requeue and formal objects

A requeue belongs to the innermost enclosing accept or protected entry body;
an intervening subprogram body cannot redirect that call. Its target is either
parameterless or has a subtype-conformant profile, including modes and null
exclusions. There is no actual parameter part. A task-entry family index is
evaluated once. Existing target visibility, entry lookup and accessibility rules
apply, including the source-formal exception to the target's lifetime rule.
An explicit or implicit dereference uses the access type's level, rather than
the access variable's declaration level.
Protected entry families and entry callable views retain their roadmap owners.

Task-to-task, task-to-protected, protected-to-task and protected-to-protected
handoffs retain one request. It owns the original caller identity, deadline,
cancellation state, exception occurrence and formal object references. Scalar
`in` formals have caller-owned storage too, so a requeue preserves their address.
Arrays retain descriptors/bounds and controlled, limited, class-wide and access
parameters retain their original objects. Only final normal completion performs
the original caller's copy-back. Exceptions and cancelled calls skip it.
Anonymous access allocator actuals belong to the original call's temporary
finalization chain, including any requeues.

The emitter evaluates the target and prepares a copied operand descriptor before
leaving the source body. Its ordinary exit plan awaits inner masters and finalizes
locals before the runtime publishes the destination request. A cleanup exception
completes the original call exceptionally instead. Internal protected requeues
join the destination queue under the same protected action. An external
protected destination executes after the runner releases the source resource;
task destinations are published after source-body cleanup.

A request has its own condition variable and releases the source task's pin at
handoff. The source task and its master may finish before the redirected call.
Protected destinations are retained from target evaluation through queue
publication, preventing source cleanup from destroying a pending destination.
Finalizing a protected object removes queued calls with `Program_Error` at their
callers; retained queue resources outlive the object until those callers wake.

Plain requeue prevents cancellation while queued. Requeue with abort permits it
again and retains the original timed/conditional deadline, rather than starting
a new timeout. Cancellation and selection are serialized; a selected body keeps
its live formals until completion. For asynchronous select, an initially selected
entry skips the abortable part only if it completes without a requeue with abort.
After that handoff the part may execute and its completion can cancel the call.
All asynchronous entry triggers use joined helper contexts with the original
logical task identity and borrowed enclosing master.

## Abort delivery and cleanup

An abort request atomically makes a task abnormal (`Is_Callable = False`),
rejects new/queued incoming calls with `Tasking_Error`, and requests abort of its
dependent tasks. Lists evaluate all names before requesting abort. Repeated
abort and abort of a completed task are harmless; a null `Task_Id` passed to
`Abort_Task` raises `Program_Error`. Task IDs retain their existing owning-master
lifetime restriction. Tasks aborted during activation do not report activation
failure to the activator, and a task aborted before starting runs no user body.

Delivery uses the select runtime's internal transfer and bypasses all Ada
handlers, including `when others`. Explicit completion points cover statement
sequences/loop iterations, calls, accepts/selects/delays, activation and supported
runtime waits. Source abort waits for a blocked cancellable operation to
acknowledge cancellation; running and deferred operations can finish later.
Selected entry calls and plain requeues cannot lose their live parameter storage.
A later requeue with abort re-enables cancellation of an already aborted caller.

Protected actions, implicit Initialize/Adjust callbacks, complete controlled
assignments and finalization remain abort-deferred. Leaving a region permits
delivery at a completion point; it does not jump through C cleanup. Unwinding
awaits dependent tasks, finalizes controlled objects exactly once and releases
masters/storage inside-out. Exceptions raised by finalization during abort do
not replace the transfer. Aborting the environment task terminates the partition
without exposing the internal transfer as an Ada exception. An abort statement
inside a protected action is a potentially blocking operation; this runtime
chooses the permitted `Program_Error` bounded-error response.

No pthread cancellation, signals or longjmp through Ada/C frames are used.
Foreign calls and host I/O that do not return to a supported completion point can
postpone abort, as in the select contract. Preemptive interruption and real-time
scheduling guarantees are not claimed.

## Evidence

`requeueentries`, `requeueparameters` and `requeuecancellation` cover mixed target
chains, family evaluation, formal identity, adapters, cleanup before handoff,
protected destination finalization, original caller identity, same-task requeue,
access targets, exceptions, Count, inherited timeouts and
asynchronous/source cancellation. `taskabort` covers task-name forms, descendant
abort, self/repeated/completed abort, status, handler bypass and abort of a
selected accepting task without unfinished copy-back. `abortcleanup`
checks deferred initialization, assignment, protected actions and failing
finalizers; `abortpartition` checks environment-task termination. All ordinary
fixtures have whole/per-unit and deterministic checks. `semantics.requeue_abort`
checks target/profile/accessibility and abort-name legality.

`runtime.requeue_lifecycle` repeats redirects, cancellation and source-master
destruction before destination completion. `runtime.task_abort_lifecycle`
repeats delay/deferred cancellation and abort during/before activation. Both
join the native ASan/UBSan and TSan controls. The sanitizer tasking campaign
includes the five ordinary source fixtures and the existing `entryexceptions`
control for initial Count-dependent barriers, with generated-memory
instrumentation under TSan. Upstream ACATS observations are audited separately; local tests are
not reported as ACATS conformance passes.


Local Darwin arm64 validation: the corrected complete suite passes 2,041/2,041
checks, including 281 ACATS-label checks. The 25 focused entry/requeue/abort
checks pass. ASan/UBSan passes 48 whole/separate tasking executions and eight
native lifecycle/deferral checks; TSan passes 50 executions including both
intentional generated-Ada race controls, plus the same eight native checks.
The repaired race control also detects 20/20 repeated whole/separate executions.
The sanitized compiler passes 307 rejection/harness/malformed-input checks.
The complete 145-case ACATS audit retains 115 C PASS, three C rejections, one C
runtime failure and 26 ungraded B observations; reviewed snapshot drift changes
no grades (17 B PASS, nine FAIL). Final harness controls and diff checks pass.
Upstream ACATS sources and `qbe/` are unchanged. Other native hosts retain their
existing validation owner.

References: Ada 2005 RM [9.5.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-5-4.html),
[9.8](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-8.html),
[9.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-4.html),
[9.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-2.html),
and AARM [9.5.1](https://www.adaic.org/resources/add_content/standards/05aarm/html/AA-9-5-1.html).
