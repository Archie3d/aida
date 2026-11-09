# Compiler roadmap

AIDA targets a dependable Ada 2005 subset, including full core tasking. Completed
milestones describe the supported surface, not full language or annex conformance.
Ada 2012 and later features are outside this target. Specialized Needs Annex
facilities and profiles need separate goals and evidence.

**Current focus:** finish Phase 8 functional/tooling work, starting with robust
artifact publication and concurrent builds. Further profiling is deferred and does not block this work.

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
  C413005 passes; remaining layouts are listed above. [Contract](docs/prefixed-views.md).
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
- [ ] **Robust artifact publication and concurrent builds.** Handle interrupted
  writes, changed/removed dependencies and search-path shadowing.
- [ ] **Driver no-op builds and cleanup.** Skip scan, bind and link when nothing
  changed; allow `--clean` without a source argument.
- [ ] **Bound prefixed temporary receiver layouts.** Support array/runtime
  aggregate receivers and subobjects of unmanaged enclosing composite function
  results. Preserve lifetime and component identity; retain explicit diagnostics
  until owning construction paths exist. [Current contract](docs/prefixed-views.md).

## Phase 9 — Full tasking

Completes Ada 2005 core tasking on top of Phases 4 and 6. Core tasking is part
of the language goal, not an optional annex. Track annex-specific priorities,
scheduling and real-time facilities separately in the Phase 8 coverage matrix.

- [ ] Task/protected aggregates and function results: extend 6e construction
  with caller-master ownership, delayed task activation and failed-result
  cleanup (`semantics.returns` retains explicit rejections).
- [ ] Task entries, `accept` statements and rendezvous, including entry
  families and parameters of every supported mode and type.
- [ ] Selective accept, with guards, `terminate` and `delay` alternatives;
  timed and conditional entry calls; asynchronous `select`.
- [ ] `requeue` and `abort`, on the Phase 6 abort-deferral design (protected
  actions, finalization).
- [ ] Task and protected discriminants, named access-to-protected-subprogram
  types (anonymous protected callbacks landed in 6d), and synchronized, task
  and protected interfaces.
- [ ] Priorities and `Storage_Size`, `Ada.Task_Attributes`, and
  `Ada.Synchronous_Task_Control`.
- [ ] Selected Systems Programming Annex support: Ada 2005 task termination
  notification (`Ada.Task_Termination`). Define separate goals and tests for
  selected Real-Time Systems Annex facilities
  such as execution-time clocks, timers, timing events and Ravenscar restrictions;
  do not infer annex or profile compliance from the minimal tasking subset.

### Phase 10 - Diagnostics quality follow-up

- [ ] **Audit and improve compiler diagnostics.** Replace ambiguous wording with
  clear explanations of the problem and, where useful, expected/actual types and
  notes pointing to related declarations. Point at the offending token or source
  span, including errors in generic instances and separately compiled units.
  Improve recovery to reduce misleading cascades without hiding independent
  errors. Add focused regression cases asserting message, file, line and column,
  with legal counterparts; preserve existing legality and ACATS grading controls.
- [ ] **Readable terminal diagnostics.** Add Rust-style source snippets with line
  numbers, highlighted error spans, caret/arrow markers and related-location
  labels. Use color when supported, with automatic terminal detection and an
  explicit override; keep plain-text output suitable for redirected logs, CI and
  existing tooling. Handle terminal width, tabs, Unicode display columns and
  multiline spans. Test both rich and plain rendering without relying on color
  alone to convey meaning.

### Phase 11 - Deferred host/performance follow-up

These remain open but do not gate the functional items above.

- [ ] **Supported-host/target follow-up.** Confirm subsequent increments on
  native hosts; retain ABI, relocation, library lookup and spaced-path controls.
  Darwin amd64 remains unvalidated. [Host contract and evidence](docs/host-validation.md).
- [ ] **Performance follow-up.** Collect matched production measurements for
  the latest offset/copy changes, perform native milestone checks and revisit
  the historical no-slowdown goal. Continue local functional work; profiling
  remains manual. [Existing evidence and policy](docs/performance-wrap-up.md).
