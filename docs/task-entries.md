# Task entries and rendezvous

Task declarations support single entries, overloaded entry profiles and entry
families. Family indices use discrete subtype marks or ranges, including dynamic
bounds evaluated once when the task declaration is elaborated. Public/private
visibility and current-instance lookup apply. Entries do not require an accept
statement; an unused entry is legal.

An `accept` statement must occur in the corresponding task body, with a fully
conforming parameter profile and an index exactly when accepting a family member.
Accepts may nest for different entries. Parameter expanded names, exception
handlers, bare return, outward goto and loop exit use the ordinary statement
rules. A return completes the innermost accept, rather than the task body.
Task-body returns outside accepts remain illegal.

## Parameters and ownership

Callers use the existing Ada argument lowering and pass a temporary packet of
its scalar values, references, array dimension records and constrained-view
metadata. The packet stays live until the rendezvous finishes. The accepting
task binds its own parameter symbols to those operands and executes the accept
body itself; caller code never executes the task's accept body.

All supported ordinary formal parameter types/modes use this path: integer,
modular, enumeration and real scalars, named object/subprogram access values,
constrained/unconstrained arrays, records, class-wide views and limited,
controlled, protected and task objects. Existing subtype/layout restrictions
still apply. Scalar `out`/`in out` copy-back and representation/array adapters
run in the caller after successful completion. An accept exception skips
copy-back; caller-side constraint failures do not propagate into the server.
Anonymous access parameters on task entries are illegal in Ada 2005.

## Queues and completion

The shared master mutex and a per-task condition protect each task's FIFO request list. Selection matches
both declaration identity and family index, preserving arrival order for each
entry. A caller waits until the selected accept completes. Queued calls and new
calls on completed tasks receive `Tasking_Error`; discarded unactivated tasks
also release queued callers. Master destruction drains established calls before
destroying their target's synchronization objects, including foreign callers.

Accept completion preserves a propagated exception in the server and sends a
separate occurrence to the caller, retaining identity, message and traceback.
Messages beyond the fixed occurrence buffer use independently owned storage.
Normal completion, exception propagation and outward control transfers finish
the rendezvous after inner masters and local finalization. Nested accepts keep
independent completion records. A selected caller cannot cancel its live packet;
asynchronous select and interrupted-master cleanup are described in the
[select contract](select-statements.md).

`E'Caller` captures the serviced caller identity for the enclosing accept.
`E'Count` reports queued calls for the current task instance, including indexed
family members; it is rejected inside a program unit nested in the task body.

## Boundaries and evidence

Task-entry renamings and entry actuals for generic formal subprograms remain
explicitly diagnosed; generic task types and ordinary generic packages containing
entry calls are supported. Entries cannot designate access-to-subprogram values.
Task/protected discriminants and interfaces, protected entry families,
requeue, abort and scheduling facilities retain their roadmap restrictions.
No tasking or annex conformance claim follows from this increment.

`taskrendezvous`, `taskentryfamilies`, `taskentryqueues`, `taskentrytypes` and
`taskentrylibrarymain` run whole-program, separate-unit and deterministic checks.
They cover parameter adapters/identity, dynamic families, overloads/defaults,
generic instances, nested/abandoned/handled accepts, caller identity, queue
counts, exceptions, completion, library units and task-valued constructors.
`semantics.task_entries` pins legality and unsupported-operation diagnostics.
`runtime.task_entry_lifecycle` checks deterministic FIFO order, long exception
messages, terminated targets and unactivated-task cancellation/draining.

Local Darwin arm64 validation ran the full 2,002-test suite, then reran reviewed
baseline changes and focused final controls. The sanitizer tasking campaigns
include all five fixtures in whole/separate modes: 26 ASan/UBSan executions and
28 TSan executions including the intentional race controls, with generated
memory-access instrumentation under TSan. Final lifecycle and 27 focused
checks passed after the baseline refresh and caller-draining repair. Unchanged ACATS C761001 passes library-task
termination before controlled library finalization; it does not exercise
rendezvous or select. B650002/B650004 still grade FAIL after accept parsing
exposes their return-legality diagnostics. Existing B-test policies and error
location groups were retained.

References: [RM 9.5.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-5-2.html),
[RM 9.5.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-5-3.html),
[RM 9.9](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-9.html).
