# Completed-feature ACATS backfill baseline

This is an observational baseline, not an expansion of what AIDA claims to
implement. It adds 122 cases to the existing 55-case strict selection:
102 executable C-tests and 20 ungraded B-test probes. The selection targets
completed roadmap areas, including cases that combine them with known
unfinished features. It is a bounded sample, not an exhaustive Ada 2005 audit.

All 124 source files (including two shared foundations) retain their original
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
has 76 C passes, 18 legal-source rejections, eight support
blockers and 20 ungraded B observations. C3A0007/C432001/C432004/C760009 now pass unchanged
after the tagged/private-view milestone. C760011 now reaches the REPORT.Ident_Char
adapter gap; C761010 reaches access-subtype and dependent-controlled-array limits. B840001's three legal extension aggregates are now
accepted; its remaining rejection diagnostics are still ungraded.

The Phase 6f backfill adds 16 C-tests covering interfaces, abstract operations,
dispatch, private/discriminated extensions and generic-instance tags. C390010
now also passes unchanged after constrained class-wide access checks and
parent/extension variant selection were repaired. All 16 join the passing CTest
selection. The original 106 observations remain unchanged.

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
