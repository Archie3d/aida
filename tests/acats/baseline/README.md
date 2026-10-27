# Completed-feature ACATS backfill baseline

This is an observational baseline, not an expansion of what AIDA claims to
implement. It adds 129 cases to the existing strict selection:
103 executable C-tests and 26 ungraded B-test probes. The selection targets
completed roadmap areas, including cases that combine them with known
unfinished features. It is a bounded sample, not an exhaustive Ada 2005 audit.

All 132 source files (including three shared foundations) retain their original
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
cannot establish correctness: the next harness stage must validate expected
error locations, permitted error groups and legal controls. B36002A now
rejects its mixed constrained/boxed array-index declarations with 12 targeted
diagnostics after the 6f-prelude repair. Its observation remains ungraded;
local `arrayindexerrors` and `arrayindexlegality` provide focused rejection
coverage and executable legal controls.

Crashes and timeouts are critical outcomes and make a probe command fail even
if previously recorded. Normal runtime failures remain RUN_FAILED; they are
not converted to the existing harness's compile XFAILs.

C460006 now passes unchanged after P6f abstract operations and record-extension
aggregate support; it joins the strict executable selection. The current cohort
has 79 C passes, 16 legal-source rejections, eight support
blockers and 26 ungraded B observations. C3A0007/C432001/C432004/C760009 now pass unchanged
after the tagged/private-view milestone. C760011 now reaches the REPORT.Ident_Char
adapter gap; C761010 reaches access-subtype and dependent-controlled-array limits. B840001's three legal extension aggregates are now
accepted; its remaining rejection diagnostics are still ungraded.

The Phase 6f backfill adds 16 C-tests covering interfaces, abstract operations,
dispatch, private/discriminated extensions and generic-instance tags. C390010
now also passes unchanged after constrained class-wide access checks and
parent/extension variant selection were repaired. All 16 join the passing CTest
selection. The original 106 observations remain unchanged.

Phase 6g promotes C393013, C640001 and C760010 to executable PASS, unchanged.
C393013 now exercises the complete controlling-result/null-extension scenario
under the ACATS label; its old frozen-rejection probe is removed. C540001's
formal-derived parser errors are gone, but expanded character-literal names
such as `Pkg.'A'` remain a separately owned 6g syntax gap. Its remaining exact
diagnostics are pinned, without claiming a pass. The other 119 observations
are unchanged.

## Reproduce

Python 3.9+ and the built compiler/toolchain are required. From the repo root:

```sh
python3 tests/acats/baseline/run.py --check --ada build/ada/ada --adac build/adac/adac --work build/tests/acats/baseline-check --jobs 4
cmake --build build --target acats_baseline
```

`--check` checks **snapshot stability**, including recorded failures. A zero
exit status means the observation matches, not that all cases passed ACATS.
Any changed result, including an improvement, requires review of the snapshot.
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
are unchanged. The current 129-case cohort has 79 executable passes, 16 compile
rejections, eight harness blockers and 26 ungraded B observations.
