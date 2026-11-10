# Select statements

Phase 9 implements selective accept, timed and conditional entry calls, and
asynchronous select for the existing task and protected entry surfaces. The
language references are Ada 2005 RM [9.7](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-7.html),
[9.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-3.html)
and [9.8](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-8.html).
Entry profiles, families, actual evaluation and parameter adapters retain the
[task-entry contract](task-entries.md).

## Selection and cancellation

A selective accept evaluates each guard once and evaluates an entry family
index or delay expression only for an open alternative. Queued matching calls
are considered before delays and else. Queues retain FIFO order for each entry;
among eligible queues the runtime chooses the earliest queued request. When
several delay alternatives have expired, source order breaks the tie. Relative
delays, Real_Time deadlines and Calendar deadlines are supported; a selective
accept's delays must all have the same relative/absolute form and time type.
All alternatives closed without else raises Program_Error.

An open terminate alternative commits when an enclosing master has completed
and every dependent task has terminated or is similarly waiting on an open
terminate alternative. The check includes lexical nesting and dependence
through an executing task. Creation, waiting, task termination and cohort
selection use the shared master registry mutex. A blocked task does not finish
its master or release its frame until its cohort commits.

Timed calls evaluate the target and actuals before the delay expression.
Conditional calls attempt immediate selection. Cancellation removes only a
queued request; a selected call must finish, including when its deadline passes.
Caller actuals and argument packets stay alive until completion. Scalar,
array and representation copy-back runs only after normal completion. Entry
exceptions retain their occurrence and bypass copy-back. Protected entries use
the same barrier and execution-resource rules as ordinary protected calls;
cancellation updates Count and reconsiders eligible barriers.

## Asynchronous transfer

A trigger is registered before its abortable part starts. An already expired
delay skips that part. An immediately selected entry skips it unless a
requeue with abort makes the call cancellable again. Otherwise, entry
completion or delay expiry requests an internal transfer which Ada handlers,
including `when others`, cannot catch. Nested selects unwind in order.

The compiler records a single cleanup block for each asynchronous operation.
Normal completion, exceptions, return, loop exit and goto all cancel or finish
the trigger before releasing its actuals. Aborted inner masters terminate their
dependent tasks and finalize controlled objects before leaving. Extended-return
results preserve their existing completion/discard rules. A trigger's following
statements run only on normal trigger completion and after the part is left.

Polling occurs at sequence entries (including loop iterations and handlers),
labeled statements, call/select/accept/delay boundaries, activation, and supported
blocking runtime waits. Protected actions, implicit Initialize/Adjust callbacks, complete controlled
assignments and controlled finalization use abort deferral; starting an asynchronous select inside an abort-deferred action
raises Program_Error. Creating a task whose owning master was entered inside
a deferred operation likewise raises Program_Error; an enclosing owner remains
eligible. Accepted callers wait for rendezvous completion rather
than abandoning a live packet. An asynchronous select inside an accept body is
supported; an accept inside its own task's abortable part is illegal.

Asynchronous entry triggers register the queue node synchronously. A joined
helper thread executes a selected entry with a private task context, the caller's
logical identity and borrowed enclosing master. This allows the abortable part
to run while the trigger is queued. Queue nodes, packets and borrowed masters
remain live until the helper is joined. No pthread cancellation or arbitrary
interruption of foreign code is used. Foreign calls and host I/O that do not
return to a supported completion point can postpone transfer; preemptive abort
and real-time scheduling guarantees are not claimed.

## Diagnostics and evidence

Guards, profiles, enclosing task ownership, alternative combinations, time
forms, non-entry calls and misplaced accepts/terminate receive diagnostics.
Statically bound entry renamings and generic callable actuals use the
[concurrent-view contract](concurrent-callable-views.md). Concurrent interfaces
and protected entry families remain open. Requeue and source
abort extend these operations through the [shared request contract](requeue-abort.md).

`selectguards`, `selectcalls`, `asyncselect`, `asynccleanup` and `selectdeferral` cover normal,
whole/per-unit and deterministic builds. They exercise guarded evaluation,
closed alternatives, all three time forms, cancellation without copy-back,
protected Count, immediate/queued triggers, nested transfers, exception bypass,
controlled cleanup, dependent tasks, extended returns and balanced default
initialization/assignment deferral on success and failure. `semantics.select`
checks legality. `runtime.select_lifecycle` repeats queue cancellation,
commitment, accept-before-delay and terminate cleanup, including a selected
call held past its timeout. The sanitizer tasking campaign includes all five
programs with ASan/UBSan and instrumented generated-Ada TSan coverage.

Unchanged upstream ACATS C85005A/C85005B now pass their complete REPORT protocol
and join the strict selection. They exercise selective rendezvous in support of
object/formal renaming and parameter aliasing; they are not entry-renaming tests.

Unchanged CXA5013 also passes and joins the regular ACATS selection. Its random
coverage test can use a delay-triggered asynchronous select to bound additional
runs; a pass does not establish that every execution takes that path.

Darwin arm64 validation ran the complete 2,022-test suite. The master-hook mock
and hello IR snapshot were refreshed for the new polling ABI and both failed
checks then passed; the three master-hook variants also passed together. Final
ASan/UBSan validation passed 36 whole/separate tasking executions and the
malformed-input campaign. TSan passed 38 executions, including both intentional
generated-Ada race controls, plus native lifecycle/deferral checks. Upstream
ACATS sources and the QBE source tree were not modified.
