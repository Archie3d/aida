# Code-generation decision: retain direct QBE emission

Status: accepted for the Phase 5 prelude. The P5 descriptor migration gate is
implemented in [array-descriptors.md](array-descriptors.md). The pressure points
below describe the pre-migration baseline. This records an architecture decision;
the implementation work below remains with its named roadmap items.

Keep the analyzed AST -> QBE emission path through the descriptor rework.
Do not introduce a separate lowered unit/function IR or a second control-flow
graph now. Make representation, call construction and scope-exit contracts
explicit inside the existing emitter as the relevant features change them.

## Evidence and alternatives

The current boundary is `Sema::analyze` followed by `QbeEmitter::emitUnit` in
[`adac/main.cpp`](../adac/main.cpp). The emitter is already split into arrays,
calls, storage, controlled results, exceptions and other feature modules.
It has reusable shape checks, array copy-back plans, storage checkpoints and
master-exit helpers. These are the starting points for further work.

The boundary is not backend independent: symbols carry QBE names and frame
offsets, values carry QBE operands, and emitters read resolved AST/type metadata.
A clean lowering layer would therefore require decisions about storage layout,
call ABI, source locations and lifetime ownership as well as a new IR.

Concrete pressure points from the review:

- [`QbeEmitter.h`](../adac/QbeEmitter.h) represents array bounds as operand
  strings in `Value`, with no stride field. `FunctionContext` carries result
  destinations, construction owners, arenas, handlers and master depths.
- [`QbeArrays.cpp`](../adac/emitter/QbeArrays.cpp),
  [`QbeCalls.cpp`](../adac/emitter/QbeCalls.cpp) and
  [`QbeFunctions.cpp`](../adac/emitter/QbeFunctions.cpp) encode descriptor or
  parameter bounds with 32-bit operations and local offset calculations.
  [`AdaArrayResult`](../runtime/adart.h) and its assertions in
  [`adart.c`](../runtime/adart.c) define the runtime result layout. Local
  descriptors, expanded array parameters and result descriptors are different
  representations; centralization must preserve their distinct roles.
- `QbeCalls.cpp` consumes result/construction state and synthesizes calls for
  inherited and renamed operations. It also selects dispatch entries and
  marshals hidden parameters, actuals and copy-backs.
- [`QbeEmitter.cpp`](../adac/emitter/QbeEmitter.cpp) adds cleanup on some
  branches, jumps and returns. `line` recognizes calls and returns from emitted
  text; [`QbeStorage.cpp`](../adac/emitter/QbeStorage.cpp) supplies checkpoints
  and inside-out master cleanup. These implicit effects need clearer APIs
  before more transfer kinds are added.

| Option | Benefit | Cost / reason for decision |
| --- | --- | --- |
| Continue direct emission unchanged | Smallest immediate patch scope | Leaves duplicated representation arithmetic and implicit lifetime effects; insufficient for the coming changes |
| Direct emission with explicit layout and operation plans (chosen) | Reuses tested ABI, ownership and cleanup mechanisms; each feature can isolate its contracts and migrate incrementally | Requires discipline: plans must replace duplicated paths and own evaluation/cleanup rules |
| Introduce a separate typed lowering IR now | Explicit values, effects and control-flow edges could support verification and future instrumentation | Broad frontend/backend migration with overlapping representations during transition; no current second backend or optimization requirement to justify that prerequisite |

This is a scope/risk judgment from the code review, not a measured performance
claim. A separate IR would still need the same layout and lifetime contracts.
Do not let emitter-local plans grow into a second AST or an unverified general
instruction list.

## Scope and feature consequences

Semantic analysis continues to own resolution, legality and static values.
The emitter owns runtime evaluation, representation/ABI marshalling and QBE
control flow. Runtime state continues to use per-task contexts, caller-owned
results, existing finalization chains and masters. Plans may refer to resolved
types/symbols and already evaluated operands; they must not redo overload
resolution or evaluate an actual/component/bound twice.

| Upcoming work | Required contract within direct emission | Roadmap owner |
| --- | --- | --- |
| Wider descriptors with strides | One named layout contract for each descriptor role and matching C boundary; array views carry data, rank, bounds, lengths, byte strides and element representation. Shared accessors calculate fields; checked size/offset arithmetic precedes allocation/access. Define null-range and conversion/sliding behavior explicitly. | P5 Array descriptors |
| Discriminant-dependent components | A component-layout operation derives address, size and active-variant checks from one evaluated enclosing object and its discriminants. Reuse array views for constrained array components; do not cache runtime layout as a static type size. | P6c runtime constraints/components delivered through [object-owned views](runtime-discriminants.md); [controlled variants](controlled-variants.md) delivered; [record clauses](representation-clauses.md) use bounded inline array views |
| Build-in-place results | A result-destination plan carries address, shape, supplied-destination status, owner/arena and construction state through calls and returns. Replace consumption of loosely related context fields with an explicit scoped contract. Retain the owned-result ABI until its separate change is tested. | P6e extended/limited results; P6f limited records |
| Cleanup and exceptions | Typed transfer/call helpers state source location, raising behavior and target scope/handler depth. One exit plan describes which masters, finalization chains and arenas are unwound, in order; finalization failure must preserve the runtime's exception rules. Do not add new cleanup behavior by matching QBE text in `line`. | P6a abort deferral, then P6e goto/returns |
| Dispatch and interfaces | A call plan separates evaluated actuals, target/static link/receiver/controlling tag, hidden result arguments, raising checks and normal-return copy-backs. Interface work extends tag lookup behind that boundary. | P6f prefixed calls/interfaces |
| Select and rendezvous | An operation plan must distinguish alternative registration, commitment, timeout/cancellation, normal completion and exceptional completion, including queue lifetime and parameter copy-back. Explicit control-flow blocks use the same exit plan; parser/legality and queue policy remain separate concerns. | P9 entry calls/select/requeue/abort |

These are implementation requirements; later feature plans do not yet exist.
The descriptor widths, physical layouts and contiguous-view restriction are now
specified by [the completed P5 ABI contract](array-descriptors.md). Later work
must explicitly revise that contract before admitting new view representations.

## Migration order and completion gates

1. **P5 static arithmetic, real edge cases and floating Value:** proceed in
   roadmap order without an IR migration prerequisite. Keep exact/static
   evaluation distinct from runtime representation; use the coverage matrix
   and ACATS harness established in the prelude.
2. **At the start of P5 Array descriptors:** first centralize the existing
   layouts and view accessors while preserving current ABI/behavior. Migrate
   locals/captures, parameter entry/actuals, result producers/consumers,
   aggregates, indexing/slicing, conversions/comparisons, attributes/streaming,
   and runtime library result helpers. Audit frame allocation in semantic
   analysis as well as emitter code. Only after all users share those
   contracts, widen bounds/lengths, add strides and revise overflow checks.
   Change compiler and runtime together and invalidate incompatible cached
   artifacts; no mixture of old and new descriptors at a call boundary.
3. **P6a, before finalization extensions:** make call effects and scope-exit
   actions explicit using the current ABI and cleanup order. Route existing
   returns, exception propagation, loop exits and master exits through those
   helpers; remove textual call/return recognition once all users migrate.
   Add abort-deferral handling there before P6c/P6e/P6f extend cleanup.
4. **P6b/P6c then P6e/P6f:** extend component layouts and result-destination
   plans only when their language prerequisites land. Introduce the call plan
   before widening dispatch/interfaces. Preserve object identity, exactly-once
   finalization, normal-return-only copy-back and existing protected lock
   release rules. Replace each migrated mechanism rather than retaining a
   permanent old/new feature switch.
5. **Before implementing P9 select:** specify its operation/exit plan against
   the runtime queue and cancellation design. If its transitions cannot be
   represented and tested through the shared call/exit APIs without duplicating
   cleanup or hidden construction state, reopen this decision before adding
   select emission. Any new IR proposal must identify that concrete gap and a
   bounded migration, not assume that wrapping QBE strings solves it.

For representation/cleanup refactors, first keep emitted ABI and behavior
stable and compare the affected generated IR before and after the extraction.
Preserve the existing whole-versus-separate determinism checks; update golden
IR only for intentional changes. At each behavior-changing step, run executable
and rejection regressions, relevant selected ACATS tests and the full suite.
Extend C runtime ABI assertions/probes when layouts change. Cover several tasks
for lifetime/runtime interactions as required by the roadmap.

Useful existing evidence includes `arrayconversions`, `arrayconversionchecks`,
`controlleddynamic`, `controlledlimitedresults`, `controlledresultfailures`,
`controlledresultdispatch`, `nestedvariants`, `interactionownership`,
`taskactivationfailure`, protected/entry runtime mode tests, and their available
determinism checks. New tests must cover newly enabled bounds/strides,
discriminant layouts, exceptional construction and select cancellation; current
tests do not prove those future behaviors. Sanitizer and generated-memory-access
instrumentation gaps remain open in Phase 8; this decision does not resolve them.

Phase 8 closes the standalone lowering-layer item under this decision. The
feature-owned implementation gates above remain required work in Phases 5, 6
and 9. Revisit also if a second backend, a concrete optimization pass or a
generated-access instrumentation design needs an independent representation.

Phase 6a update: [explicit call effects and exits](abort-deferral.md) now replace
call/return recognition in `line` and `finishFunction`. All body calls use the
shared call helper; returns record insertion points for one activation-exit plan.
Lexical master exits share the existing depth-aware helper across transfers.
The extraction preserved 83 baseline units byte for byte. Runtime deferral now
covers protected actions and existing finalization paths; Phase 9 delivery gates
remain explicit in that contract.
