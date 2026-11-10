# Task termination notification

Phase 9 supplies the Ada 2005 `Ada.Task_Termination` package: the cause enumeration,
protected handler access type, specific-handler Set/Get and dependent fallback
Set/Get. The package is preelaborated and uses the existing protected callback ABI.

A task starts with both associations clear. Its specific association takes
precedence; otherwise notification searches its enclosing task masters for the
nearest dependent fallback. A task's own dependent fallback applies to its
children, not itself. Current_Task_Fallback_Handler queries that task's stored
dependent fallback. Passing null clears an association. Specific operations reject
Null_Task_Id with Program_Error and terminated tasks with Tasking_Error. An ID
whose object no longer exists has the standard erroneous-execution restriction.

## Runtime boundary and ownership

For Ada task bodies, notification follows dependent-task waiting and local
finalization, before publishing Terminated. Completed tasks are no longer
callable during their handler. Normal completion and terminate alternatives
supply Normal; abort supplies Abnormal; escaping exceptions supply
Unhandled_Exception and the associated occurrence. An unactivated task discarded
because its creator's declarations failed is reported as Abnormal, without
executing its body. Activation exceptions are reported by the failing task and
Tasking_Error is delivered to its activator's handled sequence. A parent
task completes its own activation before initiating activation of its children
(RM 9.2), so handling a child's activation failure does not fail the parent's
activation.

The registry lock protects association changes and selection. Dispatch copies
code/link/receiver under that lock, then releases it before taking a protected
resource or running Ada. Descriptor queries retain the original immutable Ada
access value. Source accessibility checks prevent shorter-lived protected
objects from escaping into the library access type. Selection is a snapshot;
changing an association after selection does not restart notification.

The callback executes with the dying task's identity, under abort deferral and
ordinary protected-action synchronization. A borrowed context supplies this
identity when no native task thread was started. The occurrence borrows the
original message and trace for the call, without truncation or allocation;
Save_Occurrence can retain it using the usual exception API. Handler exceptions
are discarded and the original pending occurrence is restored. Task attributes
remain usable during notification and are reclaimed after termination.

Access collections follow their owner master, including allocations made by
another task. Empty lexical masters need no task registry record: collection and
accessibility records retain enough ownership information to select their
fallback. Notification is claimed once across execution, failed creation and
unactivated-task cleanup paths; joining/discarding does not repeat it.

For the environment task, the selected runtime boundary is after library tasks
have drained and before library objects and callback collections are finalized.
This keeps protected receivers alive. Consequently its specific handler observes
the pending main/elaboration/task-draining exception, but not a later exception
from a library object's finalizer. This environment boundary is a documented
limitation of the selected implementation, not a full Annex C conformance claim.

## Evidence

`tasktermination`, `terminationactivation`, `terminationcollections` and
`terminationenvironment` run in whole/separate compilation and deterministic IR
modes, and join the generated-access sanitizer campaign. They exercise specific
precedence, clearing, default/null handlers, all causes, ignored exceptions,
Current_Task and status, retained occurrences, task attributes during a callback,
local finalization, failed activation, discarded tasks and collection ownership.
The native `runtime.task_termination` test adds concurrent Set/Get, linked and
unlinked callback ABIs, a 500-byte exception message, repeated waits, and creator
exception preservation. Semantic controls reject ordinary callbacks and wrong
parameter profiles/modes; execution checks dynamic accessibility failure.

The unchanged ACATS CXC7006 source passes its complete REPORT protocol, including
multi-generation fallback selection, specific handlers and finalization and
activation exceptions. Its explicit sequential delays exceed six seconds, so its
manifest sets a bounded 15-second execution deadline instead of the harness's
five-second default. No source, expected output or grading rule was weakened.
The source's AI05-0202 finalization checks are reviewed as an Ada 2005 binding
interpretation, not an Ada 2012 syntax extension.

See [RM C.7.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-C-7-3.html),
the [ACATS observations](acats-baseline.md), and the separately scoped
[Annex follow-up goals](selected-annex-followups.md).

The full regular selection passed 2,112 checks. Final review then corrected the
parent/child activation hook ordering; all 342 tasking-focused checks passed
with that regression included. The final sanitizer selections passed 326
ASan/UBSan and 21 TSan checks, with 84 clean generated-Ada executions and 86 TSan
executions including the two required race detections. Additional sanitized
semantic controls passed, and all 161 baseline observations were rerun under
the final compiler with no drift.
