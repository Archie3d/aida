# Completed-feature ACATS backfill baseline

This is an observational baseline, not an expansion of what AIDA claims to
implement. It adds 161 cases to the existing strict selection:
135 executable C-tests and 26 ungraded B-test probes. The selection targets
completed roadmap areas, including cases that combine them with known
unfinished features. It is a bounded sample, not an exhaustive Ada 2005 audit.

All 165 source files (including four shared foundations) retain their original
bytes and rights/disclaimers.
`manifest.json` records objectives, RM areas, roadmap owners, triage notes and
SHA-256/Git-blob hashes. Sources were read directly from a GitHub archive whose
PAX comment pins revision `2785c30c1f8577d97ac7b3043882f6818781e9c5` of
[simonjwright/ACATS](https://github.com/simonjwright/ACATS/tree/2785c30c1f8577d97ac7b3043882f6818781e9c5).
That mixed-version mirror needs per-case language review. C433002/C433005
were excluded because their sources use Ada 2012 aspects/expression functions;
their hashes and exclusion reasons remain in the manifest. The Phase 6f
expansion uses Ada 95/2005 sources, including the original F392D00/F393A00
foundations for the private/abstract dispatch cases.

## Recorded observations

See [the coverage and triage report](../../../docs/acats-baseline.md).
`expected.json` retains exact normalized compiler diagnostics and executable
output, together with the recorded compiler hash and host. A compile rejection
of a legal C-test is a failure, even when it names a documented restriction.
REPORT/ImpDef/TCTouch adapter blockers are listed separately; removing one
may expose additional compiler limitations.

B-tests are compiled without linking or execution. Their observations are
`B_ACCEPTED_UNGRADED` or `B_REJECTED_UNGRADED`, never PASS. Rejection alone
cannot establish correctness. The independent [B-test grader](../../../docs/acats-grading.md)
now checks expected error locations, permitted diagnostic groups and legal
controls, recording 26 PASS (25 original foundations and one labelled local
foundation adapter). Raw observations stay ungraded. B36002A now
rejects its mixed constrained/boxed array-index declarations with 12 targeted
diagnostics after the 6f-prelude repair. Its observation remains ungraded;
local `arrayindexerrors` and `arrayindexlegality` provide focused rejection
coverage and executable legal controls.

Crashes and timeouts are critical outcomes and make a probe command fail even
if previously recorded. Normal runtime failures remain RUN_FAILED; they are
not converted to the existing harness's compile XFAILs.

C460006 now passes unchanged after P6f abstract operations and record-extension
aggregate support; it joins the strict executable selection. The current cohort
has 133 C passes, two legal-source rejections, no runtime failures and
26 raw B observations; see the current support-protocol increment in the report. C3A0007/C432001/C432004/C760009 now pass unchanged
after the tagged/private-view milestone. C760011 now passes with the existing REPORT.Ident_Char
adapter; C761010 now passes after the access-subtype and
dependent-controlled-array repairs. B840001's three legal extension aggregates are now
accepted; its remaining rejection diagnostics are still ungraded.

The Phase 6f backfill adds 16 C-tests covering interfaces, abstract operations,
dispatch, private/discriminated extensions and generic-instance tags. C390010
now also passes unchanged after constrained class-wide access checks and
parent/extension variant selection were repaired. All 16 join the passing CTest
selection. The original 106 observations remain unchanged.

Phase 6g promotes C393013, C640001 and C760010 to executable PASS, unchanged.
C393013 now exercises the complete controlling-result/null-extension scenario
under the ACATS label; its old frozen-rejection probe is removed. C540001 now
also passes unchanged after enumeration-literal actuals, expanded
character names and static related-enumeration conversions. Its complete
case-expression scenario runs as `acats.backfill_c540001`; the other 128
observations remain unchanged for that increment.

## Reproduce

Git attributes disable line-ending conversion for vendored ACATS sources so
the original-byte hashes also validate on Windows with `core.autocrlf=true`.
For an existing checkout that already converted these files to CRLF, first
preserve any local edits to the vendored sources, then restore their exact
committed bytes from an MSys2 shell after obtaining the updated `.gitattributes`.
Extracting an archive bypasses checkout line-ending conversion and Git's cached
working-tree status:

```sh
git archive HEAD tests/acats/upstream tests/acats/baseline/upstream | tar -x
python -B tests/acats/baseline/run.py --validate
```

Python 3.9+ and the built compiler/toolchain are required. From the repo root:

```sh
python3 tests/acats/baseline/run.py --check --ada build/ada/ada --adac build/adac/adac --work build/tests/acats/baseline-check --jobs 4
cmake --build build --target acats_baseline
```

`--check` checks **snapshot stability**, including recorded failures. A zero
exit status means the observation matches, not that all cases passed ACATS.
Any changed result, including an improvement, requires review of the snapshot.
Drift output includes a diff of each changed field, including compiler
diagnostics. Known source/work paths normalize Windows separators as well as
host-specific prefixes; diagnostic message text and rejection details still
compare exactly.
`--observe` writes a fresh `results.json` and per-case `observation.json` files
under `--work` without changing the committed snapshot. `--record` replaces
the complete snapshot and should be used only after reviewing changed results.
`--case ID` selects individual cases for checking/observation.

The new observed PASS cases are registered as `acats.backfill_ID` and run in
the normal suite. The metadata/hash checks and runner unit tests are separate
`baseline.*` tests. The custom target runs the complete observational cohort;
compiler/runtime failures and ungraded B observations are not registered as
passing ACATS cases. CMake explicitly reports when Python is unavailable.

Compilation and execution have separate timeouts. Each attempt uses fresh
artifacts, and timeout cleanup terminates the spawned process group on POSIX.
Source/work/repo paths are normalized in snapshots; full observation logs
remain available under the chosen work directory. The frozen failure messages
are a host baseline, not a cross-host diagnostic-format contract.

## Interface-formal legality probes (Phase 6g)

B394A01 and B394A02, with their original F394A00 foundation, are unchanged
Ada 2005 progenitor/limitedness legality scenarios, including formal interface
and formal-derived declarations. They are imported from the same pinned archive
on 2026-10-26. Their complete sources also contain unsupported concurrent
interface declarations and synchronized private/formal extensions. Rejection is
recorded exactly as B_REJECTED_UNGRADED and checked by baseline-only CTests;
it is neither a legality pass nor an executable ACATS pass. The P8 grader and
P9 syntax are still needed. Local `genericinterface*errors` isolates the rules
and legal controls without those unrelated blockers. All previous 123
observations are unchanged. BC55001 was assessed and excluded: it explicitly
checks later AI05/AI12 abstract-result rules outside the Ada 2005 target.

Synchronized-formal follow-up (Phase 6g): unchanged B394A03–B394A05 add three
ungraded probes, sharing F394A00. B394A02 loses two obsolete parser errors after
synchronized formal derived syntax is supported; the remaining 124 observations
are unchanged. Full-source rejections still include unsupported concurrent and
private-extension constructs. Local `genericsynchronized*` supplies focused
legal/illegal contract controls; the B probes do not establish legality PASS.
The 128 cases have 79 executable passes, 16 compile rejections, eight harness
blockers and 25 ungraded B observations.


## Null-default probe (Phase 6g)

BC60001 adds one unchanged Ada 2005 legality source from the same pinned archive
on 2026-10-27. Its frozen observation contains the four illegal function/abstract
null-default syntax diagnostics; parser errors prevent semantic checking of the
rest of the unit. It remains B_REJECTED_UNGRADED, not a legality pass. Local
`genericnull*` tests separately execute the legal cases and check the intrinsic
Access restriction through renaming and forwarding. All prior 128 observations
are unchanged. At that increment the 129-case cohort had 79 executable passes,
16 compile rejections, eight harness blockers and 26 ungraded B observations.

The 2026-11-06 full refresh promotes the seven discrete-range cases
(C35003A, C36104A/B, C36180A, C37002A, C41207A, C43204A). It also records
previously stale observations of existing REPORT/partial ImpDef support;
see the [current baseline](../../../docs/acats-baseline.md). No support adapter
implementation changed in this range increment.

C37003A/C37006A subsequently pass unchanged after P8 designated-record
constraints on access subtypes. Other outcomes are unchanged; B37004A's
updated diagnostics remain ungraded.

The Phase 8 adoption audit adds CD2A22E/I/J, CD2A24A/E/I/J,
C3A0019/C3A0021–C3A0024 and CC3603A/CC3605A/CC3606A/B. All 16 sources were
selected before probing and have per-case language reviews in the manifest.
The 15 executable passes join the normal ACATS label; CC3605A is the
recorded rejection for the existing access-to-array subtype constraint limit,
with a baseline-only stability test. The original 129 observations are unchanged.
The expanded totals are 107 PASS, 11 COMPILE_REJECTED, one HARNESS_BLOCKED and
26 ungraded B rejections. [Audit and completion evidence](../../../docs/acats-adoption.md).

Standard/System follow-up: C54A03A and C64005C now pass unchanged with ASCII.
C35502C now executes and fails enumeration Value tab-trimming controls; it is
RUN_FAILED, not a conformance pass. The cohort has 109 PASS, eight legal-source
rejections, one runtime failure, one support blocker and 26 ungraded B probes.

Support-protocol follow-up: unchanged C761004/C940005/C940010 now PASS after
TCTouch and local ImpDef configuration. The current totals are 112 C PASS,
six legal-source rejections, one runtime failure and 26 raw B observations.
The independent grader records 17 B PASS and nine B FAIL.

The Phase 9 requeue/abort audit retains 115 executable passes, three legal-source
rejections, one runtime failure and 26 ungraded B observations. Three stale
entry/accept parser snapshots (C432005/B650002/B650004) now expose the existing
layout/return-legality diagnostics. B grades remain 17 PASS and nine FAIL.

The concurrent discriminant/callable-view increment adds eight unchanged C-tests:
C910001/C940012/C940015, C85014B/C85018A/C85018B and C3A0018/C3A0027. All
eight PASS with complete REPORT output. The current 153-case cohort has 123 C
PASS, three legal-source rejections, one runtime failure and 26 ungraded B
observations. All 157 source hashes validate. B330001/B3A0002 now reach semantic
diagnostics; reviewed raw/grade snapshots change, but policies and their FAIL
grades remain unchanged. [Contract](../../../docs/concurrent-callable-views.md).

The concurrent-interface increment adds unchanged C940017, selected before
probing. It PASSes the complete REPORT/TCTouch protocol and joins the strict
selection. C950001/C954027 use a later Synchronization aspect and are excluded,
with their hashes/reasons recorded. Current totals: 161 cases, 124 executable
C passes, three legal-source rejections, one runtime failure and 26 ungraded B
observations; all 158 vendored hashes validate. B394A01–B394A05 now reach semantic
checking but retain FAIL under unchanged grading policies. [Contract](../../../docs/concurrent-interfaces.md).

The task-services increment selected six unchanged sources before probing:
CXC7002/CXC7003, CXDA001–CXDA003 and CXD1001. Four pass; CXC7002 retains
its packed-integer-array rejection and CXDA003 requires unsupported locking and
interrupt policies. CXDA002 uses the local Impdef.Minimum_Task_Switch allowance
(0.1 seconds), without changing upstream test logic or REPORT grading.

The task-termination increment selected unchanged CXC7006 before probing. It
passes the complete REPORT protocol. Its sequential delay groups require a
per-case 15-second execution deadline (bounded and validated); all other cases
retain the five-second default. The finalization check follows AI05-0202, an
Ada 2005 binding interpretation. No later-language syntax is introduced.

CXD1001 requests the maximum native priority. Its isolated CTest uses the
`acats_priority_probe` prerequisite: permission denial/unsupported policy yields
an explicit CTest skip, not an ACATS pass. Unexpected probe failures remain test
failures. On capable hosts the unchanged source and strict REPORT snapshot still
run. Ungated bulk observations preserve raw host-dependent outcomes, including
Tasking_Error on unprivileged Linux; the recorded snapshot is from Darwin.

Phase 12 promotes unchanged C35502C after enumeration Value was corrected to
trim surrounding spaces only. Both horizontal-tab rejection checks complete
through the strict REPORT success protocol. The full 161-case refresh changes
only this outcome; all upstream bytes and hashes are preserved.

B650004 now grades PASS unchanged after the Phase 12 nested-body return repair.
All 22 required sites match reviewed semantic families; its dedicated legal
counterpart also executes in whole-program and per-unit builds. The other 25
grades are unchanged (18 PASS / eight FAIL overall); raw B rejection observations
remain ungraded.

B3A0002 now grades PASS unchanged: generic formal subprogram Access is diagnosed
without instantiation, and the other 21 marked sites match reviewed profile,
protected-convention and intrinsic restrictions. Dedicated executable callbacks
cover the legal profiles. The other 25 grades are unchanged (19 PASS / seven
FAIL overall); raw B rejection observations remain ungraded.

B330001 now grades PASS unchanged after per-declaration diagnostic-family and
four two-line source-span reviews. A dedicated executable control retains legal
initialized and constrained declarations. Compiler diagnostics and all 161 raw
observations remain unchanged; the separate grading total is 20 PASS / six FAIL.

B650002 now grades PASS unchanged after its 32 return sites were reviewed by
return target and form. Diagnostics distinguish procedure/entry/accept targets,
missing function expressions and returns applying to an extended return. Only
this raw diagnostic stream changes; the other 25 B grades remain unchanged
(21 PASS / five FAIL overall). Upstream bytes and hashes remain intact.

B394A02–B394A05 now grade PASS after reviewing each declaration span and semantic
family: 108 required sites and six optional private completions. The independent
`interfaceancestry` control executes legal record, task/protected, private-completion
and generic ancestry in whole/per-unit modes with deterministic IR. The grading
repair changes no compiler behavior or raw observations; all 161 raw observations
and upstream hashes remain unchanged. The separate total is 25 PASS / one FAIL;
B394A01 awaits its source/foundation review. See the
[site review](../../../docs/acats-grading.md#interface-ancestry-review-phase-12).

B394A01 now grades PASS with an explicitly labelled local foundation adapter.
Official ACAA test revision 1.3 and foundation revision 1.1 confirm the missing
private-extension names; no corrected upstream pair was found. The local adapter
adds the two private extensions and their completions, preserving all vendored
bytes/hashes and raw observations. All 87 required and ten optional sites then
receive the intended specific-interface-progenitor error. Legal private-extension
ancestors and generic actuals execute in `interfaceancestry`. The separate total
is 26 PASS (25 original foundations, one adapted); this is not an unchanged
upstream B394A01 pass. See the [provenance and site review](../../../docs/acats-grading.md#progenitor-types-and-foundation-mismatch-phase-12).

C761010 and CC3605A now execute unchanged as PASS after the Phase 12
access-array constraint and controlled dependent-array follow-up. The former
also passes raw whole-program/per-unit REPORT execution. Only these two raw
observations change: 132 executable C passes, three C rejections and 26 ungraded
B observations. Upstream bytes/hashes and all 26 independent B grades are
unchanged. [Contract and validation](../../../docs/controlled-dependent-arrays.md).

### C432005 tagged concurrent components

Unchanged C432005 now passes whole-program and separate-unit REPORT execution.
Limited tagged extension aggregates reuse the existing in-place concurrent
construction and lifetime machinery. The current baseline has 133 C passes, two
C rejections and 26 raw ungraded B rejections; all upstream hashes remain intact.
See the [contract](../../../docs/tagged-concurrent-components.md).
