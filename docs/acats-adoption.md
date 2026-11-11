# Phase 8 ACATS adoption audit

This increment closes the current-subset adoption/backfill task. Feature work
continues to carry ACATS selection review, local positive/negative regressions,
implementation choices and unsupported boundaries in the
[coverage matrix](ada2005-coverage.md). It does not claim full Ada 2005 conformance
or exhaust the ACATS archive.

Subsequent Phase 12 work promotes unchanged CC3605A and C761010 after the
[access-constraint and controlled dependent-array repairs](controlled-dependent-arrays.md).
The observations and registration counts below describe the original audit.

The bounded work was to reconcile existing selection/grading, select 16 further
unchanged sources before probing them, review their language eligibility and
whole-source prerequisites, and record every result. No compiler, runtime or
support-adapter implementation changes were needed. Existing B-test grading,
predefined-environment and host/target audits keep their roadmap ownership.

## Existing selection review

| Supported family | Existing unchanged executable evidence | Remaining boundary recorded by the matrix |
| --- | --- | --- |
| Scalar types, constraints and arrays | C34001A/D, C35003A, C36104A/B, C36180A, C43204A | Dynamic/managed cross-feature layout limits |
| Records, private/tagged types and interfaces | C37003A/C37006A, C393013, C394002/C394003, C3A0013 | Concurrent interfaces, dependent controlled arrays and access-to-array constraints |
| Access values, callbacks and null exclusions | C460001/C460002, C3A2002–C3A2004, C3A0020/C3A0025 | Broader protected callback/tasking behavior |
| Expressions, statements and calls | C45347A, C45503A/C45504A, C45611A, C49022A/C, C540001/C540002, C59002A/B | Full legality/staticness grading and deferred constructs |
| Controlled lifetime and exceptions | C760010/C760011, C761002/C761006/C761012, CB41001/03/04 | TCTouch-dependent C761004 and compound task-entry tests |
| Visibility, units and categorization | C840001/C84008A/C84009A, C85011A, CA1006A, CA2009A/D, CA21001 | Serialized interfaces, artifact publication and wider categorization boundaries |
| Generic contracts and actuals | CC1204A/CC3240A, CC51B03, CC60001, CC70001–CC70003, CC3601A | Broader staticness/legal controls and existing actual-type restrictions |
| Representation and streams | CD2A22A, CD3014A/D, CD4031A, CD4051A/B, CD1009N, CDD1001/CDD2B01 | General Pack/conversion categories and nondefault bit order |
| Supported tasking/time subset | C96006A–C96008B; local task/protected and runtime tests | Full rendezvous/select/abort/interface scenarios remain Phase 9 |
| Implemented predefined libraries | CXA/CXAI/CXAIA, CXAB001, CXAG001/CXAH001/002, CXF2002/2003 selections | Predefined-environment/API audit and selected-annex boundaries |

The ordinary ACATS label admits only complete executable PASS observations and
its harness self-test. All former focused XFAILs now pass, including CC3601A and
CD2A22A; their obsolete diagnostic files are removed. Exact snapshots of rejected
C sources and ungraded B probes belong to the baseline label, which is distinct
from a conformance pass. REPORT completion/identity, nonzero exit, stderr,
crashes/timeouts and changed rejection diagnostics remain enforced by the
existing harness and runner tests.

## New cohort

All 16 sources, plus the official Length_Check helper where required, are copied
byte-for-byte from revision `2785c30c1f8577d97ac7b3043882f6818781e9c5` of the
[pinned archive](https://github.com/simonjwright/ACATS/tree/2785c30c1f8577d97ac7b3043882f6818781e9c5).
Its PAX comment verifies the revision. The
[manifest](../tests/acats/baseline/manifest.json) records source SHA-256/Git-blob
hashes, objectives, RM areas, language reviews and triage; rights/disclaimers and
original filenames/line endings are retained. There is no test-time download.

| Cases | Whole-source result | Objective |
| --- | --- | --- |
| CD2A22E/I/J | PASS | Small signed/unsigned enumeration Size, including generic actuals and official Length_Check |
| CD2A24A/E/I/J | PASS | Size plus explicit enumeration mapping, scalar/component operations and generic actuals |
| C3A0019, C3A0021–C3A0024 | PASS | Ada 2005 null exclusions on general and pool-specific access views, object/profile/default/generic boundaries |
| CC3603A | PASS | Identifier and character enumeration literals as generic subprogram actuals |
| CC3606A/B | PASS | Formal parameter defaults and the actual subprogram's parameter constraints |
| CC3605A | COMPILE_REJECTED | Whole source stops at its access-to-array subtype constraint, line 248. This is the existing limitation also recorded for C761010, not a generic-profile PASS |

These are Ada 2005 or inherited objectives. The representation files with 2014
history entries remove incompatibilities with Ada 2012; they introduce no
later-language syntax. Full-source review excludes any inference from isolated
subtests. CC3605A stays in the observation snapshot and has a baseline-only
stability test. Its existing matrix owner is unchanged; no follow-on roadmap
item is created.

The cohort adds 15 executable passes and one recorded legal-source rejection.
The original 129 observations are unchanged. The expanded 145-case baseline has
107 PASS, 11 COMPILE_REJECTED, one HARNESS_BLOCKED and 26 ungraded B rejections.
All 149 imported source hashes validate, including four official foundations.

## Final evidence

The complete Release suite passes 1,790/1,790 checks. On the recorded Darwin
arm64 host, the ordinary ACATS label contains 238 checks (237 executable C-tests
plus the grading harness), and nine baseline checks retain their separate label.
All 18 focused ASan/UBSan checks pass: the 15 added executable cases, the
CC3605A stability observation and the source-metadata/runner controls. Compiler
and C runtime instrumentation do not instrument generated Ada memory accesses;
LeakSanitizer is disabled on this host.

The full 145-case refresh validates source provenance and has no crashes,
timeouts or executable failures. All original 129 observations compare equal
to the preceding committed snapshot, and the compiler fingerprint is unchanged.
Every one of the 17 newly imported source/dependency files compares byte-for-byte
with the pinned archive. CTest registration review confirms that CC3605A has
only the baseline label and all 15 new PASS cases have the ACATS/backfill labels.
