# Compiler roadmap

AIDA targets a dependable Ada 2005 subset, including full core tasking. Completed
milestones describe the supported surface, not full language or annex conformance.
Ada 2012 and later features are outside this target. Specialized Needs Annex
facilities and profiles need separate goals and evidence.

**Current focus:** Phase 11 compiler and driver review fixes are complete.
Phase 12 addresses ACATS failures in the supported subset. Phase 13 host
validation and performance follow-up remain deferred.
Further Annex work has separately scoped goals and acceptance tests.

- [Coverage and current limits](docs/ada2005-coverage.md)
- [Detailed implementation history and historical measurements](docs/roadmap-history.md)
- [Predefined-environment inventory](docs/predefined-environment.md)

## Working rules

1. Diagnose unsupported constructs; never silently drop their semantics.
2. Add focused execution and legality tests, deterministic and separate-unit
   checks where applicable, and relevant ACATS controls. Keep the full suite green;
   use sanitizer and concurrent-call checks for affected lifetimes/runtime state.
3. Reuse descriptors, result ownership, finalization, generic contracts and
   per-task context. New mutable runtime state must be task-safe.
4. Prioritize correctness before speed. Keep `qbe/` unchanged.
5. Maintain the coverage matrix and feature contracts; keep README.md brief and
   truthful. Mark completion only after recording restrictions and validation.
6. Follow phase dependencies and resolve local prerequisites before dependents.
   Keep implementation-defined choices, selected-test passes and actual
   conformance claims distinct.

## Completed milestones — Phases 1–7

| Phase | Delivered scope | Details |
| --- | --- | --- |
| 1. Stop silent acceptance | Diagnostics and recovery for unsupported syntax, pragmas, representation and generic categories | [History](docs/roadmap-history.md#phase-1--stop-silent-acceptance) |
| 2. Tasking foundations | Per-task exception/trace/storage context, result buffers, synchronized tables, threading and masters | [History](docs/roadmap-history.md#phase-2--tasking-foundations) |
| 3. Harden the subset | Unit loading/binding, elaboration, visibility, conformance, aggregates, conversions, variants and checked attributes | [History](docs/roadmap-history.md#phase-3--harden-the-existing-subset) |
| 4. Minimal tasking | Entry-free tasks, protected operations/entries, time/delays, atomics and task identity | [History](docs/roadmap-history.md#phase-4--minimal-tasking) |
| 5. Numeric/representation foundations | Exact folding, checked real/integer arithmetic, modular/fixed point, 64-bit array descriptors and representation clauses | [Arithmetic](docs/static-arithmetic.md), [representation](docs/representation-clauses.md) |
| 6. Core language milestones | Dynamic data/discriminants, access and limited types, program structure, tagged types/interfaces, generics and check policies (6a–6h); supported-area ACATS repairs | [History](docs/roadmap-history.md#phase-6--complete-the-core-language), [generics](docs/generic-types.md), [checks](docs/pragmas-and-checks.md) |
| 7. Predefined library | Strings/containers, host/calendar facilities, streams, exceptions, wide characters/text, categorization and foreign ownership | [Library audit](docs/wide-library-audit.md), [foreign results](docs/foreign-results.md) |

Existing controlled results, accessibility, coextensions, scoped array storage,
per-unit compilation, manifest binding and incremental caches are foundations to
extend. Supported profiles and remaining restrictions live in the coverage matrix
and contracts, rather than being repeated in this roadmap.

## Phase 8 — Functional work and tooling

- [x] Ada 2005 coverage matrix, supported-surface audit, selected ACATS adoption
  and B-test grading; non-tasking legality findings repaired.
  [Coverage](docs/ada2005-coverage.md), [ACATS](docs/acats-adoption.md),
  [grading](docs/acats-grading.md), [legality](docs/legality-audit.md).
- [x] Packed/unaligned representation, changed derived-record layouts, freezing,
  Pack/bit storage, discrete-range syntax, access-subtype constraints, generic
  child loading and grouped formals.
  [Representation](docs/representation-clauses.md), [packed arrays](docs/packed-arrays.md),
  [ranges](docs/discrete-ranges.md), [access constraints](docs/access-subtype-constraints.md),
  [generic children](docs/generic-children.md), [grouped formals](docs/generic-grouped.md).
- [x] Finite process/test deadlines, malformed-source campaigns, recovery,
  sanitizer presets and external generated-access instrumentation.
  [Robustness and coverage limits](docs/robustness.md).
- [x] Predefined-environment audit and missing facilities: Standard/System,
  storage/address conversions and pools, random generators, dispatching
  constructors, Stream_IO/Text_Streams and composite equality repairs.
  [Inventory](docs/predefined-environment.md),
  [implementation history](docs/roadmap-history.md#predefined-library-implementation).
- [x] Native ABI/install/path validation and CI fixes on active Linux amd64/arm64
  and MSYS2 UCRT64 hosts; Darwin arm64 validated locally. Separate routine
  build/test CI from manual profiling. [Host evidence](docs/host-validation.md).
- [x] Implement the measured master, allocation/registry, aggregate-storage,
  constant-offset and bounded-copy repairs. Further measurements are deferred;
  historical no-slowdown is not claimed. [Batch wrap-up](docs/performance-wrap-up.md).
- [x] Bound prefixed-view renaming and generic actuals for supported tagged
  receivers, including master/library lifetime and reduced profiles. Unchanged
  C413005 passes. [Contract](docs/prefixed-views.md).
- [x] Ordered external object/archive linkage with normal Ada binding and runtime
  ownership. [Driver regression](tests/CheckExternalLinkage.cmake).
- [x] Keep direct QBE lowering with explicit ownership/cleanup mechanisms.
  Reopen the decision before select emission if cancellation/cleanup paths need
  duplication, or when another backend needs a shared representation.
  [Lowering decision](docs/lowering-decision.md).
- [x] **Hierarchical unit source layouts.** Support deterministic unit-to-path
  mapping for both flat (`ada-text_io.ads`) and hierarchical (`ada/text_io.ads`)
  layouts, including bodies and deeper children, without recursive scanning.
  Preserve search-directory precedence, allow mixed layouts, diagnose conflicting
  files within a search directory and verify declared unit identities. Cover
  separate compilation, caching and relocated installs. Runtime sources migrated
  into subfolders after loader support was validated.
  [Contract and validation](docs/unit-source-layouts.md).
- [x] **Serialized semantic interfaces.** Source-free clients use versioned,
  compiler/ABI-checked `.asi` replay interfaces with `.ali` build metadata and
  compiled IR. Import reconstructs semantic state and conservatively retains
  body dependencies. [Contract and validation](docs/semantic-interfaces.md).
- [x] **Robust artifact publication and concurrent builds.** Atomic file
  replacement, inherited directory locks, checked multi-file commit records,
  exact dependency/provenance contracts and fixed scan-root replay. Interrupted
  groups fail closed and regenerate from sources.
  [Contract and validation](docs/artifact-publication.md).
- [x] **Driver no-op builds and cleanup.** Checked whole-build receipts skip
  all tools for unchanged file outputs. Content/provenance, lookup candidates,
  tools, environment and required products invalidate stale receipts; `--force`
  bypasses the shortcut. Source-free cleanup accepts `-D` or `-o`, coordinates
  with active builds and reports filesystem errors.
  [Contract and validation](docs/driver-builds.md).
- [x] **Bound prefixed temporary receiver layouts.** Retain complete enclosing
  values for array/runtime aggregates and unmanaged composite function results,
  including selections, slices and tagged conversions. Reuse ordinary object
  construction, descriptors and master/library cleanup; preserve single
  evaluation and constant-view legality. [Contract and validation](docs/prefixed-views.md).

## Phase 9 — Full tasking

Completes Ada 2005 core tasking on top of Phases 4 and 6. Core tasking is part
of the language goal, not an optional annex. Track annex-specific priorities,
scheduling and real-time facilities separately in the Phase 8 coverage matrix.

- [x] Task/protected aggregates and function results: extend 6e construction
  with caller-master ownership, delayed task activation and failed-result
  cleanup (`semantics.returns` retains copy-rejection controls).
  [Contract, restrictions and validation](docs/concurrent-results.md).
- [x] Task entries, `accept` statements and rendezvous, including entry
  families and parameters of every supported mode and type.
  [Contract, remaining boundaries and validation](docs/task-entries.md).
- [x] Selective accept, with guards, `terminate` and `delay` alternatives;
  timed and conditional entry calls; asynchronous `select`.
  [Contract, restrictions and validation](docs/select-statements.md).
- [x] `requeue` and `abort`, on the Phase 6 abort-deferral design (protected
  actions, finalization).
  [Contract, restrictions and validation](docs/requeue-abort.md).
- [x] Task and protected discriminants, including per-object dependent private
  components and task entry-family bounds.
- [x] Task-entry and protected-operation callable views: renamings and generic
  subprogram actuals/defaults, including statically bound entry selects.
- [x] Named access-to-protected-subprogram types (anonymous protected callbacks
  landed in 6d).
  [Contract, restrictions and validation](docs/concurrent-callable-views.md).
- [x] Synchronized, task and protected interfaces, including concurrent
  implementations, private completions, generic actuals and dispatching selects.
  [Contract, restrictions and validation](docs/concurrent-interfaces.md).
- [x] Priorities and `Storage_Size`, `Ada.Task_Attributes`, and
  `Ada.Synchronous_Task_Control`.
  [Contract, native scheduling limits and validation](docs/task-services.md).
- [x] Selected Systems Programming Annex support: Ada 2005 task termination
  notification (`Ada.Task_Termination`).
  [Contract, environment boundary and validation](docs/task-termination.md).
  [Separate goals and tests](docs/selected-annex-followups.md) cover further
  Systems Programming and Real-Time Systems Annex facilities, including
  execution-time clocks, timers, timing events and Ravenscar restrictions;
  tasking support does not imply annex or profile compliance.

### Phase 10 - Diagnostics quality follow-up

- [x] **Audit and improve compiler diagnostics.** Replace ambiguous wording with
  clear explanations of the problem and, where useful, expected/actual types and
  notes pointing to related declarations. Point at the offending token or source
  span, including errors in generic instances and separately compiled units.
  Improve recovery to reduce misleading cascades without hiding independent
  errors. Add focused regression cases asserting message, file, line and column,
  with legal counterparts; preserve existing legality and ACATS grading controls.
  [Audit, recovery and validation](docs/diagnostics.md).
- [x] **Readable terminal diagnostics.** Add Rust-style source snippets with line
  numbers, highlighted error spans, caret/arrow markers and related-location
  labels. Use color when supported, with automatic terminal detection and an
  explicit override; keep plain-text output suitable for redirected logs, CI and
  existing tooling. Handle terminal width, tabs, Unicode display columns and
  multiline spans. Test both rich and plain rendering without relying on color
  alone to convey meaning. [Options, rendering contract and limits](docs/diagnostics.md).

### Phase 11 - Compiler and driver review fixes

The Darwin arm64 Release review passed all 2,114 existing CTest checks. Separate
probes exposed the following gaps. Exercise the code generation fixes in both
whole-program and per-unit builds.

- [x] **Skip null for-loop ranges before narrowing the bounds.** An eight-bit
  loop variable can turn a starting bound of 256 into zero, causing a null
  range to execute once. Branch to the loop exit using the original bounds.
  Check zero iterations for forward `256 .. 0` and reverse `0 .. -256` ranges,
  and one iteration for a valid singleton.
- [x] **Invalidate native objects when the selected tool changes.** Switching
  `PATH` to a different compiler under the same `CC` name currently relinks
  stale objects. Use the resolved QBE and C compiler fingerprints in the
  per-object cache stamp. Verify that switching either tool recompiles the
  affected objects and that an unchanged toolchain still reuses them.
- [x] **Use unsigned ordering for modular array elements.** A one-element array
  containing `Word'Last` compares below one containing zero for
  `type Word is mod 2 ** 64`. Use the element type's signedness when comparing
  the first unequal pair. Check that array and scalar ordering agree in both
  directions across the high bit.
- [x] **Preserve signed zero in floating-point negation and absolute value.**
  Dynamic negation currently leaves positive zero positive, and `abs` leaves
  negative zero negative. Flip the sign for negation and clear it for `abs`.
  Verify both zero signs with variable operands and `Copy_Sign` for `Float`
  and `Long_Float`.
- [x] **Diagnose mixed logical operators without parentheses.** The parser
  accepts `True or False and False`. Enforce the Ada 2005 expression grammar
  at each nesting level and point the diagnostic at the conflicting operator.
  Cover short-circuit forms, repeated operators and parenthesized legal cases.
- [x] **Allow assembly and IR output in a writable directory.** With a
  read-only parent, `-S` and `--emit-ir` fail because the output lock is
  created one directory above the requested output directory. Place the guard
  where the writer has access. Test both modes under these permissions and
  verify that concurrent writers to the same output still serialize.

Validation on Darwin arm64 (11 November 2026): the full Release run passed
2,114 checks; its sole failure was the expected loop IR snapshot, refreshed and
verified to contain only the null-range branch and identifier renumbering. All
16 final affected checks passed, covering that snapshot and all six fixes;
focused diagnostics/review checks also passed under ASan/UBSan. Regression:
[`semantics.review_fixes`](tests/review/check.py), with
[whole-program/per-unit execution cases](tests/ada/interactions/reviewfixes.adb).
No additional host or performance claims are made.

### Phase 12 - ACATS correctness and legality follow-up

At `bcb8a92` on Darwin arm64, all 295 regular ACATS-labelled CTest checks
passed. The separate [B-test grader](docs/acats-grading.md) reported 17 passes
and nine failures. Of the six known nonpassing [backfill C-tests](docs/acats-baseline.md),
one failed at runtime and five reached documented feature limits.

- [ ] **C35502C — enumeration `'Value`.** Trim leading and trailing spaces
  only. Horizontal tabs currently pass through the same whitespace trimming
  and incorrectly yield an enumeration value. Verify that either tab raises
  `Constraint_Error`, ordinary spaces remain accepted, and the unchanged case
  completes its REPORT protocol.
- [ ] **B650004 — returns in nested package bodies.** Check which construct a
  return applies to. Reject returns in package elaboration even inside an
  enclosing callable construct. Cover the ten missed error sites and preserve
  legal returns directly inside accept statements and subprogram bodies.
- [ ] **B3A0002 — `'Access` of a generic formal procedure.** Diagnose the
  illegal attribute in the generic body at line 168 even without an
  instantiation. Review the diagnostic groups at the other 21 marked sites
  and verify the legal callback assignments.
- [ ] **B330001 — indefinite objects and components.** Review the diagnostic
  families and accepted source spans for each declaration. Four array errors
  point to the opening line immediately before the marker. Update the
  [grading policy](tests/acats/grading/policies.json) for verified matches
  and retain the legal initialized and constrained declarations as controls.
- [ ] **B650002 — return diagnostics.** All 32 required sites receive errors.
  Explain why an extended return is illegal in a procedure, entry body or
  accept statement. Check each diagnostic against its return rule before
  updating the accepted messages, including returns inside an extended return.
- [ ] **B394A02–B394A05 — interface ancestry.** Review the reason for each
  rejection within its upstream declaration span. Account for explicitly
  optional diagnostics on private completions in B394A02 and B394A03. Add
  legal ancestry controls and promote each case once it passes the grader.
- [ ] **B394A01 — reconcile the test and its foundation.** The pinned test
  refers to `Non_Lim_Intf_Priv` and `Lim_Intf_Priv`, which the pinned
  `F394A00` omits. Verify a matching upstream pair and its provenance, then
  rerun the case and grade the intended progenitor errors.
- [ ] **Refresh the ACATS baseline table.** Correct the stale rejection rows
  for C85005A and C85005B, which now execute successfully. Record newly
  verified grades and executable passes as the fixes land.

The remaining C-test rejections stay with their documented limits. CC3605A
needs access-to-array subtype constraints, and C761010 also needs controlled
elements in discriminant-dependent arrays. C432005 needs tagged records with
task and protected components, CXC7002 needs packed signed-integer arrays, and
CXDA003 needs `Locking_Policy` and `Interrupt_Priority`. Focused cases CD30002,
CD72A02 and CDB0A01 retain their alignment, address-conversion and positive
access-type `Storage_Size` limits, respectively.

### Phase 13 - Deferred host/performance follow-up

These remain open but do not gate the functional items above.

- [ ] **Supported-host/target follow-up.** Confirm subsequent increments on
  native hosts; retain ABI, relocation, library lookup and spaced-path controls.
  Darwin amd64 remains unvalidated. [Host contract and evidence](docs/host-validation.md).
- [ ] **Performance follow-up.** Collect matched production measurements for
  the latest offset/copy changes, perform native milestone checks and revisit
  the historical no-slowdown goal. Continue local functional work; profiling
  remains manual. [Existing evidence and policy](docs/performance-wrap-up.md).
