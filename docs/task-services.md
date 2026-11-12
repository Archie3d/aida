# Task services and native priorities

Phase 9 supplies task stack configuration, base-priority controls, task
attributes and suspension objects. This is a selected Ada 2005 surface, not a
claim of conformance to the Systems Programming or Real-Time Systems Annex.

## Priority and stack configuration

`pragma Priority (Expression)` is accepted immediately in task/protected
specifications and subprogram bodies. Its expected type is `Integer`; task and
protected expressions are evaluated per object and may use discriminants. The
subprogram form must be static. Only the selected main subprogram's pragma has
an effect: the binder applies it before library elaboration, so library tasks
inherit the correct initial base priority. Other subprogram pragmas are checked
but have no effect, as required by D.1.

`System.Priority` is 0 .. 30, `Any_Priority` is 0 .. 31, and the default is 15.
`Ada.Dynamic_Priorities` provides the Ada 2005 Set_Priority/Get_Priority profiles.
Creation inherits the creator's base priority unless explicitly configured.
Activation and rendezvous temporarily inherit the activator/call priority;
explicit protected ceilings are checked and inherited for the action, including
entry execution and nested actions. Get_Priority reports the base value. Null
identities raise Program_Error; Get on a terminated task raises Tasking_Error,
while Set on one has no effect. An identity must still denote a live task object.

Native urgency uses Mach thread precedence on macOS and per-thread nice values
on Linux, relative to the process's initial nice value. MSYS2/winpthreads maps
normal Ada priorities around Windows THREAD_PRIORITY_NORMAL using discrete native
bands, with TIME_CRITICAL reserved for interrupt priority 31. Other pthread hosts use
the current native scheduling policy when it has enough priority levels; they
reject nondefault priorities otherwise. These are host scheduling mechanisms,
with no deterministic dispatch or latency guarantee. Permission failures and
unsupported native priority changes raise Tasking_Error; the runtime does not
publish a successful base-priority change when the native update failed.
Linux commonly requires privilege to raise urgency, including restoring it after
a temporary decrease. This target restriction also applies to inheritance.

Locking_Policy (Ceiling_Locking/Native_Locking) and Interrupt_Priority follow the
[hosted ceiling-locking contract](ceiling-locking.md), including partition-wide
configuration and capability checks. Linux can explicitly select FIFO/RR with
AIDA_TASK_SCHEDULER. Task_Dispatching_Policy, Priority_Queuing, interrupt
attachment/masking and protected current-instance Priority attributes remain
diagnosed. Partitions without a locking policy retain native mutex defaults;
Ceiling_Locking supplies ceiling 30 when no explicit priority is given. Full
Annex D scheduling compliance is unclaimed. Entry queues
retain FIFO selection. Asynchronous entry helpers keep the caller's Ada identity
and their own native thread's inheritance state.

`pragma Storage_Size (Expression)` is accepted only in a task definition, with
an expression of any integer type evaluated at object creation. The native stack
reservation is at least the platform's default pthread stack size and at least
the requested byte count, rounded up to a host page boundary. Zero requests use
the native default; negative, overflowing or unsupported reservations raise
Storage_Error. Task `Object'Storage_Size`, including implicit access dereference
and a task body's current instance, reports this reservation in storage elements
(bytes), excluding the task control block. Winpthreads' zero default is resolved
from the executable's PE stack reservation and Windows allocation granularity.
Default reservations preserve lazy commit; larger winpthreads commit requests
use the OS's MiB reservation rounding and its unsigned-int size limit.
Access-type Storage_Size/Storage_Pool
retain their separate existing semantics.

## Task attributes

`Ada.Task_Attributes` is a source generic with the standard definite nonlimited
formal type and Initial_Value object. Values are allocated lazily, as permitted
by C.7.2(28). Value, Reference, Set_Value and Reinitialize support explicit task
identities and default Current_Task. Set_Value preserves a previously returned
handle; Reinitialize may destroy its object and invalidate handles. A null
identity raises Program_Error; a terminated identity raises Tasking_Error.

Each instance serializes its operations with one mutex. The lock covers Ada
copying/assignment and is released by a limited controlled guard on normal and
exceptional exits. Raw Attribute_Handle dereferences need user synchronization;
concurrent dereferences/operations or use after reinitialization, termination or
instance finalization have the RM's erroneous-execution restrictions. Reentering
an instance from its controlled Adjust/Finalize operations can block forever,
within the bounded-error permission of C.7.2.

Values are finalized and their storage reclaimed at task termination or instance
finalization, whichever occurs first. The runtime pins instances during operations
and walks a serial-number snapshot during termination, so finalizers may close
other instances or create new ones without leaving stale registry pointers.
Finalization uses the ordinary allocation/finalization machinery, including its
exception rules and continued cleanup after a user finalizer raises. There is
no fixed attribute-count or value-storage limit beyond available memory and the
runtime's ordinary allocation limits. No real-time execution-cost metric is
claimed for these heap-backed, host-scheduled operations.

## Suspension objects

`Ada.Synchronous_Task_Control` is preelaborated, with a limited by-reference,
preelaborably initialized Suspension_Object. Objects start False and lazily
allocate a mutex/condition resource. Set_True, Set_False, Current_State and
Suspend_Until_True synchronize publication and access. A stored True is consumed
by a wait. Set_True releases one blocked task and immediately leaves False; a
later Set_False cannot revoke the delivered permit. A second simultaneous waiter
raises Program_Error.

Set_True/Set_False can run during a protected action. Suspend_Until_True is
potentially blocking and raises Program_Error there. Waiting polls the existing
abort/asynchronous-select mechanism at bounded intervals; cancellation removes
the waiter before releasing its resource reference. Controlled object cleanup
drains references and destroys the resource. This includes suspension objects in
records, arrays and allocated objects through ordinary finalization.

## Validation

Five Ada fixtures exercise the services in whole/separate compilation and with
deterministic IR: `taskservices`, `taskpriority`, `taskprioritylibrarymain`,
`taskattributescleanup`, and `suspensioncontrol`. Native lifecycle tests cover
shared attribute locking, registry reentry during finalization, exception cleanup,
stack rounding, priority failures and suspension publication/wakeup. Semantic
controls cover pragma placement, duplicates, types, ranges, task current-instance
queries and limited generic actuals. The sanitizer tasking campaign includes all
five fixtures and native service tests.

Six upstream ACATS sources were selected before probing, with unchanged bytes
and REPORT protocol. See the [reviewed observations](acats-baseline.md) for passes
and remaining failures. References: [D.1 priorities](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-D-1.html),
[13.3 stack storage](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-3.html),
[C.7.2 attributes](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-C-7-2.html),
and [D.10 suspension](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-D-10.html).

The regular selection has 2,097 checks. Its initial full run passed 2,096; the
remaining hello IR snapshot was updated for the intentional binder/priority data
change and its rerun passed. After the final access-to-task Storage_Size and
protected-priority preelaboration fixes, all 53 focused checks passed, with
additional sanitized semantic/native checks. The sanitizer selections passed
325 ASan/UBSan and 20 TSan checks; generated tasking covered 76 clean executions
and 78 TSan executions including both required race detections. All six selected
ACATS outcomes were reobserved under the final compiler with no drift.

CXD1001 and CXDA003 isolated CTests first run native maximum-priority capability probes
in a disposable process. Permission denial or an unsupported scheduler reports
CTest skip (77), with no ACATS PASS recorded. A successful probe runs the complete
unchanged test and strict snapshot comparison. Unexpected probe errors, crashes
and timeouts fail. Raw bulk observations retain their actual outcomes and remain
host-specific; the probe does not rewrite the Darwin snapshot or upstream source.

The CXDA003 follow-up extends the probe through interrupt priority and native
restoration. Compiler and runtime changes require matching rebuilt artifacts.
See [the contract and validation](ceiling-locking.md).
