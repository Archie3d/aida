# Compiler roadmap

Phases 1–12 are complete for AIDA’s documented Ada 2005 subset, including core
tasking. Phase 13 is the only open phase. Additional Specialized Needs Annex
facilities and profiles have [separate goals](docs/selected-annex-followups.md).

## Phase 12 - C761010 correctness follow-up

- [x] **C761010 — controlled aggregate initialization.** Access-to-array
  constraints and deferred indefinite constants are repaired. Controlled
  discriminant-dependent arrays use independent storage/finalization chains,
  construct aggregates without Adjust and clean up partial failures. The
  binder prefers parent bodies before child bodies when dependency ordering
  permits. Unchanged C761010 passes whole/per-unit REPORT execution; CC3605A
  also passes after the shared access-constraint repair. Dedicated execution,
  determinism and legality controls cover capture, copies/assignment,
  initialization/Adjust failures and elaboration fallback. Task/protected
  dependent-array elements and represented managed arrays remain diagnosed.
  [Contract](docs/controlled-dependent-arrays.md). The baseline now records
  132 C passes, three C rejections and 26 raw B rejections; only C761010 and
  CC3605A change. All 165 upstream hashes and 26 independent B grades remain
  intact. Darwin arm64 validation: all 2,170 Release checks pass (308 ACATS),
  plus 42 expanded ASan/UBSan checks and three final concurrent rechecks in
  each build. Four workers independently exercise ownership and failure paths.
  The complete baseline recheck and `git diff --check` pass; QBE is unchanged.

## Phase 13 - Deferred host/performance follow-up

- [ ] **Supported-host/target follow-up.** Confirm subsequent increments on
  native hosts; retain ABI, relocation, library lookup and spaced-path controls.
  Darwin amd64 remains unvalidated. [Host contract and evidence](docs/host-validation.md).
- [ ] **Performance follow-up.** Collect matched production measurements for
  the latest offset/copy changes, perform native milestone checks and revisit
  the historical no-slowdown goal. Continue local functional work; profiling
  remains manual. [Existing evidence and policy](docs/performance-wrap-up.md).

## Completed work

The [coverage matrix](docs/ada2005-coverage.md) describes supported features and
limits. The [roadmap history](docs/roadmap-history.md) and
[predefined-environment inventory](docs/predefined-environment.md) cover the
language, runtime, library and tooling milestones. Phases 10–12 completed
the [diagnostics audit](docs/diagnostics.md), compiler and driver fixes, and
the [ACATS correctness and legality review](docs/acats-grading.md).

The latest Phase 12 Release run on Darwin arm64 passed all 2,170 checks,
including all 308 ACATS checks. The C761010 follow-up also passed 42 expanded
ASan/UBSan checks and final concurrent execution/determinism rechecks.
[ACATS results and remaining restrictions](docs/acats-baseline.md).

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
