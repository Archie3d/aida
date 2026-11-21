# Compiler roadmap

Phases 1–12 are complete for AIDA’s documented Ada 2005 subset, including core
tasking. Phase 13 adds C interfacing packages; Phase 14 retains deferred
host/performance work. Additional Specialized Needs Annex facilities and profiles
have [separate goals](docs/selected-annex-followups.md).

## Phase 13 - Interfaces.C support

Target Ada 2005 `Interfaces.C` (plural), its `Strings` and `Pointers` children,
and the parent `Interfaces` foundation. Follow
[RM B.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-B-2.html),
[B.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-B-3.html),
[B.3.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-B-3-1.html)
and [B.3.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-B-3-2.html).

Existing scalar C imports and native function pointers provide a starting point
([current contract](docs/subprogram-access.md)). The packages are absent; this
requires compiler representation and ABI work as well as library bodies. In
particular, unsigned 32-bit storage currently widens to 64 bits, array bounds
use signed 64-bit descriptors, and floating point stops at binary64. The shared
runtime-call path also passes array lengths, so it cannot directly serve C array
parameters. Complete the following in dependency order:

- [x] **Target contract and ABI probes.** Record C sizes, alignments,
  signedness and floating formats for each supported target, including LP64
  versus Windows LLP64, `wchar_t`, `size_t` and `ptrdiff_t`. Probe with the target
  C compiler; do not infer a cross-target ABI from the build host. Decide how
  to support native `long double` without changing QBE; diagnose unsupported
  formats rather than silently aliasing them to `double`. Record package and
  target completion criteria before implementation. [Contract and probe
  evidence](docs/interfaces-c-abi.md).
- [ ] **Representation prerequisites.** Add exact-width signed/unsigned
  storage and correct loads, stores, conversions and argument/result extension.
  Resolve full-width modular array indices for `size_t` without truncating upper
  bounds; retain checked allocation extents and migrate descriptors/caches if
  needed. Support addressable aliased character elements with native strides.
  Validate layout against C, including high-bit values and boundary failures.
- [ ] **Parent and scalar packages.** Add `Interfaces` integer/modular
  types, shift/rotate operations and hardware floating types, then `Interfaces.C`
  numeric types, implementation-defined constants and character types. Preserve package purity,
  distinct type identity, overload resolution, generic matching and separate-unit
  metadata. Register installed sources and predefined inventory entries. Keep
  target-dependent floating formats explicitly gated by 13a's contract.
- [ ] **Character arrays and conversions.** Implement narrow, wide,
  `char16_t` and `char32_t` arrays and the conversion/termination APIs. Cover
  functional and buffer forms, counts, nonzero lower bounds, null arrays,
  embedded/missing NULs, short targets and character-mapping failures. Reuse
  caller-owned array results and cleanup paths; preserve aliased element identity.
- [ ] **Native C call boundary.** Separate foreign C marshalling from
  internal runtime helper conventions. Pass compatible arrays as element
  pointers without hidden Ada lengths/descriptors; preserve mutability and
  lifetime. Extend convention checking and checked interfaces for the package
  types, access values and existing imported callbacks. Prove direct/indirect
  scalar, pointer and array calls against C helpers in whole/per-unit builds;
  diagnose unsupported profiles before emission.
- [ ] **Interfaces.C.Strings.** Implement native `chars_ptr`, null/default
  initialization, borrowed array views, allocation/free, length/value and update
  operations. Distinguish borrowed and owned storage and document foreign
  allocator/lifetime obligations. Cover required exceptions, allocation failure,
  bounded reads/updates and cleanup, with sanitizer and concurrent-call controls.
- [ ] **Interfaces.C.Pointers.** Implement generic native element pointers,
  stride-aware arithmetic/difference, terminated and counted values/copies.
  Preserve generic contracts and aliased storage; test null-pointer exceptions,
  element sizes, nonzero index origins, terminators and copy limits. Document
  caller obligations and erroneous-execution boundaries without promising bounds
  checks that native pointers cannot provide.
- [ ] **Integration and completion audit.** Add focused Ada/C examples,
  legality, determinism, installation and separate-unit checks throughout these
  steps; select relevant unchanged ACATS tests and grade them independently.
  Confirm new ABI paths on native supported hosts, run the full suite and relevant
  sanitizers, and update coverage, inventory and feature contracts. Report any
  target/API gaps explicitly; passing a subset does not complete all of B.3.

General `Export`, Ada-defined C callbacks, imported/exported objects, by-value
records/`C_Pass_By_Copy`, variadics and C++ bindings are separate follow-ups;
implement only prerequisites needed by the package scope here. Phase 14's broader
host/performance backlog does not replace this phase's native ABI validation.

## Phase 14 - Deferred host/performance follow-up

- [ ] **Supported-host/target follow-up.** Confirm subsequent increments on
  native hosts; retain ABI, relocation, library lookup and spaced-path controls.
  Darwin amd64 remains unvalidated. [Host contract and evidence](docs/host-validation.md).
- [ ] **Performance follow-up.** Collect matched production measurements for
  the latest offset/copy changes, perform native milestone checks and revisit
  the historical no-slowdown goal. Continue local functional work; profiling
  remains manual. [Existing evidence and policy](docs/performance-wrap-up.md).

## Completed work

- [x] **Phases 1–11.** Language, runtime, library, core tasking, tooling and
  diagnostics milestones for the documented Ada 2005 subset.
- [x] **Phase 12 — ACATS correctness follow-ups.** Controlled aggregate
  initialization, tagged concurrent components, packed signed arrays and hosted
  ceiling locking. The selected baseline has 135 C passes, no C rejections and
  26 independently graded B tests. [Results and restrictions](docs/acats-baseline.md).

See the [roadmap history](docs/roadmap-history.md) for completed milestones and
validation evidence, the [coverage matrix](docs/ada2005-coverage.md) for current
support limits, and the [predefined-environment inventory](docs/predefined-environment.md)
for library coverage. Selected-test passes do not establish full Ada conformance.

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
