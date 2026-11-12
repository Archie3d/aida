# Selected Annex follow-up goals

Phase 9 closes the documented core tasking and selected task-service work.
Implementing a package or passing selected ACATS cases does not establish
conformance to an entire Specialized Needs Annex or a profile. The following
work requires separate scope, target contracts and acceptance evidence.

| Goal | Required scope | Acceptance tests and evidence |
| --- | --- | --- |
| Execution-time clocks (`Ada.Execution_Time`) | Define per-Ada-task CPU accounting, native helper-thread attribution, resolution, range and unsupported-target behavior. | Compare CPU-bound and blocked tasks; exercise concurrent queries, termination, overflow and monotonicity; publish target measurements. |
| Execution-time timers and budgets | Define timer ownership, clock accounting, protected handler dispatch, replacement, cancellation and shutdown. | Deterministic clock/expiry controls, cancellation races, task abort and finalization, repeated expiry, multiple timers and sanitizer lifetime tests. |
| Timing events (`Ada.Real_Time.Timing_Events`) | Define monotonic deadlines, protected handler execution, cancellation, object lifetime and supported dispatch guarantees. | Fake-clock ordering and boundary tests; native integration; replacement/cancellation races; handler exceptions, shutdown and sanitizer tests. |
| Ravenscar restrictions | Select an Ada-version/profile contract and enforce every required restriction and policy on explicitly supported targets. | Independent positive/negative controls for each restriction, binding-time checks, queue/resource bounds and target scheduling measurements. No profile claim based only on source acceptance. |
| Complete dispatching and interrupt policies | Hosted Ceiling_Locking and Interrupt_Priority are supplied by the [selected contract](ceiling-locking.md). Full native scheduling guarantees, dispatch/queue policies and interrupt attachment/masking require additional target contracts. | Ordering/inheritance tests including dynamic priority changes, rendezvous and protected actions; interrupt lifecycle/failure controls; multi-processor and stress measurements. |
| Complete Annex C termination/environment semantics | Retain safe notification resources through complete environment finalization, including failures in library finalizers, and review remaining Annex C facilities independently. | Environment normal/unhandled/finalization-error controls, heap-backed handler receivers, shutdown ordering, unchanged broader ACATS selection and sanitizer lifetime checks. |

These goals are deferred, not implemented by the current phase. Existing
unsupported packages, pragmas and profiles must continue to produce diagnostics.
The native scheduling limits in [task services](task-services.md) and environment
notification boundary in [task termination](task-termination.md) remain explicit.
