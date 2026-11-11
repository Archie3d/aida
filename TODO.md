# Compiler roadmap

Phases 1–12 are complete for AIDA’s documented Ada 2005 subset, including core
tasking. Phase 13 is the only open phase. Additional Specialized Needs Annex
facilities and profiles have [separate goals](docs/selected-annex-followups.md).

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

The final Phase 12 Release run on Darwin arm64 passed 2,155 of 2,156 checks,
including all 305 ACATS checks. The external C compiler timed out during
`runtime.containerfamilies_modes`, which passed on an isolated rerun.
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
