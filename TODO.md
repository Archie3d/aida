# Compiler roadmap

Phases 1–12 are complete for AIDA’s documented Ada 2005 subset, including core
tasking. Phase 13 is the only open phase. Additional Specialized Needs Annex
facilities and profiles have [separate goals](docs/selected-annex-followups.md).

## Phase 12 - ACATS correctness follow-ups

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
- [x] **C432005 — tagged concurrent components.** Enable limited tagged records
  and extensions containing task/protected components using existing layout,
  in-place construction, activation and cleanup machinery. Unchanged C432005
  passes whole/per-unit REPORT execution. Focused controls cover three tagged
  levels, controlled finalization, inherited views, protected exceptions, four
  concurrent callers, noncopying diagnostics and nonlimited-ancestor legality.
  [Contract](docs/tagged-concurrent-components.md). Darwin arm64 validation:
  all 2,176 Release checks pass (310 ACATS), all 17 focused ASan/UBSan checks
  and all 312 legality controls pass. The full 161-case baseline recheck and
  26 independent B grades pass. Only C432005 changes: 133 C passes, two C
  rejections and 26 raw B rejections. All upstream hashes remain intact; QBE
  is unchanged.
- [x] **CXC7002 — packed signed arrays.** Generalize packed array descriptors
  to 1–64-bit integer component widths, with signed reads, cross-byte masked
  writes, checked byte extents and explicit component Size minima. Unchanged CXC7002 passes whole/separate-unit
  REPORT execution. Focused concurrent controls cover copies, results, nested
  arrays, scalar copy-back, logical streams, native snapshot validation and
  explicit unsupported boundaries.
  [Contract](docs/packed-arrays.md). Darwin arm64 validation: all 2,182
  Release checks pass (312 ACATS, including 26 B grades), plus 29 focused
  ASan/UBSan checks. All 313 legality fixtures pass. The full 161-case baseline
  recheck matches: only CXC7002 changes, giving 134 C passes, one C rejection
  (CXDA003) and 26 raw B rejections. Source hashes and QBE remain unchanged.
- [x] **CXDA003 — hosted ceiling locking and interrupt priorities.** Retain
  partition policies through scans, checked interfaces and compiled metadata;
  bind them before elaboration. Default ceilings, nested inheritance, queued
  wakeups/cancellation and native failures preserve task context and lock lifetime.
  Linux offers explicit FIFO/RR selection; Windows reserves a distinct interrupt
  band. Hosted scheduling limits and native capability skips remain explicit.
  [Contract](docs/ceiling-locking.md). Darwin arm64 validation: all 2,197
  Release checks pass (314 ACATS), plus all 36 focused ASan/UBSan checks.
  The 161-case baseline recheck and all 26 independent B grades pass: 135 C
  passes and no C rejections. Only CXDA003 changes. All upstream hashes remain
  intact, the baseline tables reconcile and QBE is unchanged. The user confirms that CI builds and tests
  pass on all configured platforms. Privileged Linux FIFO/RR execution remains
  subject to native scheduling capability.

## Phase 13 - Deferred host/performance follow-up

- [ ] **Supported-host/target follow-up.** Confirm subsequent increments on
  native hosts; retain ABI, relocation, library lookup and spaced-path controls.
  Darwin amd64 remains unvalidated. [Host contract and evidence](docs/host-validation.md).
- [ ] **Performance follow-up.** Collect matched production measurements for
  the latest offset/copy changes, perform native milestone checks and revisit
  the historical no-slowdown goal. Continue local functional work; profiling
  remains manual. [Existing evidence and policy](docs/performance-wrap-up.md).

## Completed work

- [x] **IDE temporary-string finalization scaling.** Index heap finalization
  chain ownership alongside allocation addresses and stop successful local
  removal before searching shared registrations. Preserve runtime ABI,
  task synchronization, deferred publication and optional-index failure
  fallbacks. Expanded runtime controls cover ownership, resize/failure,
  arbitrary cleanup and four concurrent workers. The original IDE reproducer
  retains checksum 12000; local Darwin arm64 medians stay near 7.9 ms at
  0/1,000/6,000 live allocations, versus 195.4 ms previously at 6,000.
  [Contract and matched evidence](docs/finalization-scaling.md). All 50 focused
  Release checks pass; the full 2,208 checks pass with one rebuild-guard retry.
  Both expanded runtime CTests pass under ASan/UBSan and TSan;
  `git diff --check` passes. QBE is unchanged. Native host confirmation and
  the broader Phase 13 performance goal remain open.

- [x] **Private numeric overload compatibility.** Universal numeric expressions
  match only visibly numeric types; private integer/real/fixed completions cannot
  contribute literal or named-number overload candidates in clients. The IDE
  real-time visibility reproducer compiles, integer width arithmetic computes
  18, constructed span arithmetic remains valid and genuine public numeric
  ambiguities remain diagnosed. Tests also cover illegal private initialization
  and legal full-view uses ([contract](docs/operator-visibility.md)). Darwin
  arm64 validation: all 2,208 Release checks pass (314 ACATS), all 321 selected
  diagnostic/resolution checks pass and all 11 focused ASan/UBSan checks pass.
  Whole/per-unit execution and deterministic output are covered;
  `git diff --check` passes. QBE is unchanged.

- [x] **IDE input and entry-point regressions.** Get_Line/Skip_Line stop at
  LF on open terminals and pipes; optional page-marker lookahead waits for the
  next ordinary text read. Immediate/raw reads retain control bytes and regular
  files retain eager page accounting ([contract](docs/text-io.md)). Ada `Main`
  uses a distinct linker symbol from the binder's C entry point. Both original
  IDE reproducers pass, including open-pipe/POSIX pseudo-terminal input; six
  focused Release and six ASan/UBSan checks pass, covering whole/separate-unit
  linking, narrow/wide lines, empty/full buffers and immediate/stream transitions.
  Darwin arm64 full-suite validation: 2,199/2,200 pass initially; C3A0027 detects
  a concurrent compiler rebuild and passes on retry. All seven final focused
  checks (including C3A0027) pass; `git diff --check` passes. QBE is unchanged.

The [coverage matrix](docs/ada2005-coverage.md) describes supported features and
limits. The [roadmap history](docs/roadmap-history.md) and
[predefined-environment inventory](docs/predefined-environment.md) cover the
language, runtime, library and tooling milestones. Phases 10–12 completed
the [diagnostics audit](docs/diagnostics.md), compiler and driver fixes, and
the [ACATS correctness and legality review](docs/acats-grading.md).

The latest Phase 12 Release run on Darwin arm64 passed all 2,197 checks,
including all 314 ACATS checks. The CXDA003 follow-up also passed 36 focused
ASan/UBSan checks. The CXC7002 follow-up passed 29 focused
ASan/UBSan checks and all 313 legality fixtures. The C432005 follow-up also passed 17 focused
ASan/UBSan checks and 312 legality controls. The earlier C761010 follow-up
passed 42 expanded ASan/UBSan checks and concurrent rechecks.
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
