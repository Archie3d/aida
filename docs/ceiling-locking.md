# Hosted ceiling locking and interrupt priorities

This increment supplies the Ada priority/ceiling model on the existing native
thread runtime. It is selected hosted functionality, with the scheduling limits
below; it does not establish full Annex D or Ravenscar conformance.

## Configuration and compilation

`pragma Locking_Policy (Ceiling_Locking)` is supported before the first library
unit or subunit in a source file. `Native_Locking` is an implementation-defined
alternative retaining the previous default behavior. A policy declared in any
unit applies to the entire partition. Repeated matching settings are accepted;
conflicts are diagnosed, including conflicts between independently compiled
units. Other policy names and misplaced/trailing configuration pragmas are
rejected. There is no separate configuration-only-file option.

Scans, checked semantic interfaces and compiled-unit records retain the policy.
Binding checks it against compiled metadata, so editing a manifest cannot silently
change a compiled unit's policy. Source-free dependencies and subunits retain the
setting. The binder selects it before library elaboration and task creation.

Under Ceiling_Locking, an unspecified protected-object ceiling is
`System.Priority'Last` (30). Native_Locking, and partitions without an explicit
policy, retain unspecified native mutex locking for such objects. Explicit
protected priorities keep their existing ceiling checks and inheritance under
both policies. Entry queues retain FIFO selection.

## Priority pragmas

`Interrupt_Priority [(Expression)]` is accepted in task and protected definitions,
mutually exclusive with `Priority`. A missing expression means
`System.Interrupt_Priority'Last`. Expressions have expected type Integer and are
evaluated per object, including discriminant-dependent expressions.

The target model remains Priority 0 .. 30 and Interrupt_Priority 31 .. 31.
Ordinary task Priority expressions are converted to Priority; task
Interrupt_Priority expressions are converted to Any_Priority (0 .. 31).
Protected Priority expressions are converted to Any_Priority; protected
Interrupt_Priority expressions are converted to Interrupt_Priority. Invalid
values raise Constraint_Error during creation. These distinctions follow
[RM D.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-D-1.html)
and [RM D.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-D-3.html).

A caller whose active priority exceeds a protected object's ceiling receives
Program_Error before waiting for its execution resource. Protected actions inherit
the ceiling; nested actions, exceptions, entry servicing, requeue and cancellation
restore the inherited state and recompute the effective priority from the current
base priority. Queued legacy entries reacquire their ceiling on wakeup, and
cancellation-related barrier evaluations run as protected actions. Pending
exception messages and traces survive that evaluation.

Execution actions retain the protected resource through mutex release and native
priority restoration. Failed native updates raise Tasking_Error; a failed base
priority change is not published, and failed inheritance is rolled back. Native
restoration failures remain errors rather than successful priority changes.
Set_True/Set_False on suspension objects remain usable during interrupt-priority
protected actions. Hardware interrupt attachment/masking, Task_Dispatching_Policy,
Priority_Queuing and protected current-instance Priority attributes remain
unsupported and diagnosed.

## Host scheduling

`AIDA_TASK_SCHEDULER` is captured once, on the first native priority operation.
An unset/empty value or `host` selects the existing hosted backend. Linux also
accepts `fifo` and `rr`. Unsupported settings fail explicitly; there is no silent
fallback or automatic privilege escalation.

| Host/backend | Mapping and limits |
| --- | --- |
| macOS host | Mach thread precedence relative to the default priority. This is a scheduling influence, without a deterministic dispatch or hard-real-time guarantee. |
| Linux host | Per-thread nice values relative to the initial runtime baseline. Raising urgency requires suitable permissions/limits, including restoration after a temporary decrease. Ceiling_Locking usually needs those permissions even for ordinary protected objects, since their default ceiling is 30. |
| Linux fifo/rr | Explicit SCHED_FIFO/SCHED_RR selection, mapping all 32 Ada levels monotonically to distinct native priorities between the policy's reported minimum and maximum. CAP_SYS_NICE or sufficient RLIMIT_RTPRIO is required; priority 31 maps to the native maximum. This does not implement hardware interrupt masking or a complete Ada dispatching policy. |
| Windows/MSYS2 host | winpthreads/Windows relative thread priorities: 0 maps to IDLE, 1–7 LOWEST, 8–14 BELOW_NORMAL, 15 NORMAL, 16–22 ABOVE_NORMAL, 23–30 HIGHEST, and 31 TIME_CRITICAL. The interrupt level now has a distinct higher native band. Normal levels still share bands. The process priority class is not changed. |

These mechanisms do not give identical scheduling guarantees across platforms.
In particular, priority 31 denotes the hosted interrupt-priority range; it does
not raise Windows kernel IRQL or mask the host's hardware interrupts.

## Validation

Unchanged CXDA003 executes the full REPORT protocol in whole and separate-unit
modes. Priority-dependent CTests first probe native application and restoration
of the required priority. Permission denial/unsupported scheduling produces an
explicit skip, without recording an ACATS pass. Unexpected failures, malformed
probe responses and noisy success are test failures. Raw bulk baseline runs
retain their actual host outcomes; the recorded snapshot is Darwin arm64 evidence.

`ceilinglocking` checks a default ceiling during library elaboration, policy
inheritance from a dependency, nested ceiling violations, exceptions, suspension
operations, base-priority changes and four independent task callers.
`nativelocking` retains the previous default behavior. `interruptbounds` checks
per-object range failures and the distinct task/protected rules without needing
privileged native priorities. Determinism and whole/separate-unit controls cover
these paths.

`semantics.ceiling_locking` checks configuration placement, conflicting policies,
compiled metadata consistency, source-free dependencies, subunits and native-probe
skip/failure boundaries. Native lifecycle tests exercise queued wakeups and
conditional/timed cancellation in four threads, including barrier active-priority
checks. Failure controls verify rollback and cleanup when native scheduling fails.
Production Linux and Windows backend branches are exercised through API models,
including denied updates, frozen configuration and reserved interrupt-band mapping.
Native Linux real-time and Windows confirmation remains part of host follow-up.
Darwin arm64 validation passed all 2,197 Release checks (314 ACATS), all 36
focused ASan/UBSan checks, the complete 161-case baseline recheck and all 26
independent B grades. All pinned upstream hashes remain intact; the case/area/grade
tables reconcile, `git diff --check` passes and QBE is unchanged.
