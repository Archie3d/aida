# ACATS completed-feature baseline — 2026-10-26

This backfill adds **128 unchanged upstream cases**: 103 C-tests and 25 B-test
probes, alongside the strict focused selection. The recorded host is Darwin
arm64. The compiler build hash, exact observations and source provenance are in
[the snapshot](../tests/acats/baseline/expected.json) and
[selection manifest](../tests/acats/baseline/manifest.json).
This is a baseline of the current subset, not a conformance claim.

| Observation | Cases |
| --- | ---: |
| B accepted, ungraded | 0 |
| B rejected, ungraded | 25 |
| Legal C-test rejected | 16 |
| Support/harness blocked | 8 |
| Executable PASS | 79 |
| Executable failure | 0 |

No crashes or timeouts were observed. **16 executable C-cases fail**, all compile
rejections; eight further C-cases are blocked by
support adapters.
All 25 rejected B-probes remain ungraded, not legality passes. B36002A now
rejects its mixed constrained/unconstrained array definitions; the Phase 8
B-test grader remains required before any upstream legality-pass claim.

## Interface-formal observations (Phase 6g)

B394A01/B394A02 and F394A00 add two unchanged Ada 2005 legality probes for
progenitor kinds and limited descendants, including generic formals. The whole
sources reject on unsupported concurrent syntax as well, so their observations
remain ungraded, with exact diagnostics under the baseline label. They are not
legality passes. All original 123 observations, including the 79 executable
passes, are unchanged. Local `genericinterface*` covers the isolated legality
rules, legal controls, dispatch, category matching, callbacks and nested/library
replay; existing C394002/C394003 retain their strict inheritance passes.

Synchronized-derived-formal follow-up adds unchanged B394A03–B394A05 for task,
protected and synchronized ancestry, including formal extensions. They remain
ungraded because full sources also contain unsupported Phase 9 constructs.
B394A02 loses two obsolete formal-syntax parser diagnostics; the other 124
previous observations are unchanged. `genericsynchronized*` isolates the new
static contracts, legal controls, tags/callbacks and nested/library replay.

## Reviewed repairs

Phase 6g promotes C393013, C640001 and C760010 to executable PASS, unchanged.
C393013 runs the complete null-extension controlling-result scenario through
generic and private extensions, including private overrides. C640001 exercises
tagged generic callbacks and redispatch. C760010 verifies controlled lifetimes
and explicit/implicit exception propagation through tagged private formals.
C393013 now uses `acats.backfill_c393013`; its frozen-rejection probe is removed.
C540001 retains only its independent expanded character-literal syntax rejection
(`Pkg.'A'`), owned by the following 6g enumeration-literal work. Its complete
remaining diagnostics are reviewed and pinned. The other 119 observations are
unchanged; the current cohort has 79 executable passes.

The Phase 6f backfill adds 16 C-tests from the same pinned archive, preserving
all 18 imported files byte-for-byte, including the F392D00/F393A00 foundations.
Fifteen initially passed unchanged and joined the normal suite: C394002/C394003 (limited and
nonlimited interfaces), C393001/C393012/C393A02/C393A03/C393A06 (abstract
operations and dispatch), C392004/C392010/C392011/C392D01/C392D02 (conversions,
controlling parameters/results and private primitives), C391001/C391003
(discriminated/local extensions) and C390011 (generic-instance tags).
C390010 now also passes unchanged and joins the normal suite. Constrained
class-wide views retain captured discriminant constraints and check access/value
boundaries; parent and extension variants select independently, including
`others`, and ancestor discriminants initialize after their child dependencies.
`classwideconstraints` and `taggedvariantgroups` cover the repairs in whole/per-unit/
deterministic modes with four workers. All original 106 observations remain
unchanged at that milestone; the expanded cohort then had 76 executable passes.

Before this backfill, the final Phase 6f interface milestone retained all 106 observations:
60 executable passes, 18 legal-source rejections, eight support blockers and
20 ungraded B observations. Its local interface scenarios are documented in the
[interface contract](interfaces.md); no new upstream pass is inferred from them.

The tagged/private-view milestone promotes unchanged C3A0007, C432001, C432004
and C760009 to PASS. C760011 now reaches REPORT.Ident_Char (P8); C761010 gets
past unknown-discriminant views and exposes access-subtype and dependent
controlled-array restrictions. The cohort is now 60 PASS, 18 compile rejections,
eight harness blockers and 20 ungraded B observations. Strict-selection
C3A0009 and C761012 also pass unchanged; C3A0013 retains only the P8
range-attribute array-declaration XFAIL. See the
[tagged/private-view contract](tagged-discriminants.md).

The next 6f overriding/null/access-result milestone retains all 106 exact
observations: 56 executable passes, 23 legal-source rejections, seven support
blockers and 20 ungraded B observations. No additional case is promoted by this
change; the snapshot's compiler fingerprint is refreshed. Focused execution and
legality scenarios are documented in the
[overriding/null-procedure contract](overriding-null-procedures.md).

C460006 now passes unchanged after the first 6f item. Abstract operations use
inherited dispatch slots, concrete types must override abstract obligations,
and supported record extensions accept ordinary record aggregates. The case's
view-conversion, object-renaming and assignment checks pass. `abstractsubprograms`,
`abstractinheritance` and `abstract*errors` provide focused execution and legality
controls. B840001 remains ungraded; three legal record-extension aggregates
no longer produce unsupported-aggregate diagnostics.

All five original executable failures now pass unchanged.

C34001A/C34001D/C34002A now pass unchanged after the 6f-prelude repair:
scalar `'Value` returns and checks `S'Base` (RM 3.5(53)), rather than the
first subtype's static or captured bounds. `derivedscalaroperations` covers
base-range operations, inherited constraints, conversions, receiving-context
checks and four tasks in whole/per-unit/deterministic modes. The snapshot and
strict ACATS selection include these three reviewed promotions.

C761006 also passes unchanged after disjoint controlled assignment began
adjusting the actual target directly (RM 7.6(21), 7.6.1(16)). Overlapping
assignments retain owned snapshots. `controlledadjustment` checks component and
parent failures, cleanup/recovery, overlapping slices, initialization and
deallocation, including class-wide assignment and four tasks. Runtime ownership
checks and the Ada regression pass with ASan/UBSan on the C runtime.

C840001/C84008A/C84009A now pass after the operator-visibility repair.
Predefined operators participate in scope hiding; expanded calls resolve
implicit operators, and use type follows the type's identity through subtypes.
`operatorvisibility` and `operator*visibilityerrors` cover private/body/child
contexts, qualified calls, homographs and diagnostics in whole/per-unit/
deterministic modes, with four tasks and compiler ASan/UBSan checks.

B840001 remains ungraded. Additional missing-use diagnostics at lines 266, 270
and 290 correspond to upstream ERROR markers; private-child legal controls
remain accepted. This is not a substitute for the Phase 8 B-test grader.
C730004 now also passes after the full-view limitedness repair described below.

Legal-source rejections still need review: some hit explicit later roadmap
features, while others expose missing behavior in completed areas.

B36002A now produces 12 diagnostics for its mixed constrained/boxed index
definitions, at the first mismatching index of each declaration (RM 3.6(2–6)).
The shared parser check also covers anonymous arrays and generic formals.
`arrayindexerrors` covers both orders, subtype marks, raw ranges, enumeration/
Boolean indices, three dimensions and recovery; `arrayindexlegality` checks
legal constrained/boxed definitions, runtime bounds, null arrays and generic
instantiations in whole/per-unit/deterministic modes. Compiler ASan/UBSan
checks pass. The B observation remains ungraded; no strict ACATS C-case is
promoted by this legality repair.

C83022A/C83024A and related C64004G/C64005A/C64005B now pass unchanged
after enclosing-name lookup was repaired (RM 4.1.3(10–13)). `enclosingnames`
and `enclosingnameerrors` cover hidden objects, parameters, defaults, nested
blocks/loops, generic current instances, callable-prefix ambiguity and
non-enclosing/value-use rejections, including captured frames and four tasks
in whole/per-unit/deterministic modes. Compiler ASan/UBSan checks pass for
these fixtures and all five unchanged upstream cases.
C41103A now also passes after resolving array/function prefixes and preserving
slice bounds, as described below. C54A07A and C64005C retain REPORT
Ident_Char blockers; C64005C also needs the predefined ASCII package (P7/P8).
Their changed diagnostics are recorded without claiming executable passes.

C540002 now passes unchanged after selector resolution began applying its
any-discrete-type context before binding (RM 5.4(4)). `caseselector` and
`caseselectorerrors` cover direct/generic calls, discrete scalar categories,
subtype preservation, single evaluation, private views and genuine ambiguity;
choices do not supply selector context. Whole/per-unit/deterministic execution,
four tasks and compiler ASan/UBSan checks pass.

C49022C now passes unchanged after adding the mixed root real/integer
multiplying profiles (RM 4.5.5(17)). Converted named numbers expose their
universal numeric type (RM 3.3.2). `realnumbers` and `realnumbererrors` cover
exact arithmetic, conversion boundaries, fixed contexts, forbidden profiles,
static failures and evaluation limits in whole/per-unit/deterministic modes,
with four tasks and compiler ASan/UBSan. The strict selection includes this
reviewed promotion.

C41103A/C41105A/C730004/C73002A now pass unchanged. Array-result prefixes
participate in index/call overload resolution; nested slice indexing retains
its own lower bound. Nested package bodies see enclosing body declarations
without leaking them to clients. Concatenation/equality use a visible nonlimited
full view. Visible partial-view discriminants and class-wide use-type marks are
also repaired; C37006A and C760011 retain independently owned prerequisites.
`supportedrejections`/`supportedrejectionerrors` cover legal and rejection
controls in whole/per-unit/deterministic modes, with four tasks and compiler
ASan/UBSan. All four new passes join the strict selection.

The [remaining-rejections audit](supported-rejections.md) maps every one of the
23 legal-source rejections and seven support blockers to concrete prerequisites
in P6f, P6g, P7, P8 or P9. No unclassified front-end rejection remains in this
bounded baseline; removing a prerequisite still requires rerunning the
unchanged compound case before any promotion.

## Coverage by area

Counts are observations, not percentages of RM coverage. Compound cases may
be blocked before reaching the feature named in their objective.

| Area | C PASS | C rejected | C runtime failure | Harness blocked | B ungraded |
| --- | ---: | ---: | ---: | ---: | ---: |
| derived scalars | 8 | 1 | 0 | 1 | 0 |
| arrays discriminants | 4 | 6 | 0 | 0 | 0 |
| object orientation | 16 | 1 | 0 | 0 | 0 |
| access callbacks | 6 | 1 | 0 | 0 | 0 |
| indexing slices | 6 | 1 | 0 | 1 | 0 |
| aggregates | 0 | 4 | 0 | 0 | 0 |
| conversions static | 6 | 1 | 0 | 0 | 0 |
| control flow | 1 | 2 | 0 | 1 | 0 |
| calls returns | 6 | 1 | 0 | 1 | 0 |
| private finalization | 6 | 5 | 0 | 1 | 0 |
| visibility renaming | 9 | 2 | 0 | 0 | 0 |
| loading subunits | 3 | 0 | 0 | 0 | 0 |
| protected objects | 0 | 0 | 0 | 2 | 0 |
| legality sample | 0 | 0 | 0 | 0 | 20 |

## Follow-up order

1. Investigate the remaining legal C rejections; turn
   confirmed defects into focused compiler regressions before fixing them.
2. Implement B-test grading against upstream error markers, permitted error
   groups and legal controls. Include recovery/diagnostic coverage; a nonzero
   compiler exit or an unrelated unsupported construct must not yield PASS.
3. Fill the small REPORT/TCTouch gaps, then configure applicable ImpDef
   support. Never stub target-dependent semantics merely to compile a case.
   The protected probes also need later library/tasking features.
4. Review the remaining legal-source rejections by RM requirement and roadmap
   owner. Retain explicit unsupported-feature boundaries and promote cases as
   features or defects are resolved.

The baseline runner self-tests enforce strict REPORT completion/identity,
zero-exit failure detection, ungraded B results, support-gap separation,
crash/timeout handling and source/snapshot integrity. Only the 79 observed
executable PASS cases join the normal ACATS test label. The full cohort has a
separate snapshot-stability target; matching a recorded failure is not an
ACATS pass. [Commands and grading boundaries](../tests/acats/baseline/README.md).

## Case inventory

Each objective, source hash and owning feature is recorded in the manifest.
The table below gives the observed status and initial triage for every case.
“Front-end rejection to investigate” is not a confirmed root-cause diagnosis.

| Case / upstream source | RM area | Observation | Initial triage / next owner |
| --- | --- | --- | --- |
| [C34001A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c34001a.ada) | 3.4–3.5 | Executable PASS | scalar Value base-range repair; P6f-prelude |
| [C34001C](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c34001c.ada) | 3.4–3.5 | Executable PASS | covered probe; P3/P5 |
| [C34001D](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c34001d.ada) | 3.4–3.5 | Executable PASS | scalar Value base-range repair; P6f-prelude |
| [C34002A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c34002a.ada) | 3.4–3.5 | Executable PASS | scalar Value base-range repair; P6f-prelude |
| [C34002C](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c34002c.ada) | 3.4–3.5 | Executable PASS | covered probe; P3/P5 |
| [C35003A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c35003a.ada) | 3.4–3.5 | Legal C-test rejected | documented language gap; P8 discrete-range syntax audit |
| [C35003D](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c35003d.ada) | 3.4–3.5 | Executable PASS | covered probe; P3/P5 |
| [C35502A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c35502a.ada) | 3.4–3.5 | Executable PASS | covered probe; P3/P5 |
| [C35502C](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c35502c.ada) | 3.4–3.5 | Support/harness blocked | support adapter gap; P8 REPORT adapter |
| [C35502G](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c35502g.ada) | 3.4–3.5 | Executable PASS | covered probe; P3/P5 |
| [C36104A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c36104a.ada) | 3.6–3.8 | Legal C-test rejected | documented language gap; P8 discrete-range syntax audit |
| [C36104B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c36104b.ada) | 3.6–3.8 | Legal C-test rejected | documented language gap; P8 discrete-range syntax audit |
| [C36180A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c36180a.ada) | 3.6–3.8 | Legal C-test rejected | documented language gap; P8 discrete-range syntax audit |
| [C37002A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37002a.ada) | 3.6–3.8 | Legal C-test rejected | documented language gap; P8 discrete-range syntax audit |
| [C37003A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37003a.ada) | 3.6–3.8 | Legal C-test rejected | documented language gap; P8 access-subtype constraints |
| [C37003B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37003b.ada) | 3.6–3.8 | Executable PASS | covered probe; P3/P6b/P6c |
| [C37005A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37005a.ada) | 3.6–3.8 | Executable PASS | covered probe; P3/P6b/P6c |
| [C37006A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37006a.ada) | 3.6–3.8 | Legal C-test rejected | documented language gap; P8 access-subtype constraints |
| [C37008A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37008a.ada) | 3.6–3.8 | Executable PASS | covered probe; P3/P6b/P6c |
| [C37008B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37008b.ada) | 3.6–3.8 | Executable PASS | covered probe; P3/P6b/P6c |
| [C390010](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c390010.a) | 3.4/3.9/3.10 | Executable PASS | constrained class-wide/variant repair; P6f-prelude |
| [C390011](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c390011.a) | 3.4/3.9/3.10 | Executable PASS | covered probe; P6f object orientation |
| [C391001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c391001.a) | 3.7/3.9.1 | Executable PASS | covered probe; P6f object orientation |
| [C391003](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c391003.a) | 3.7/3.9.1 | Executable PASS | covered probe; P6f object orientation |
| [C392004](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c392004.a) | 3.9.2/7.3 | Executable PASS | covered probe; P6f object orientation |
| [C392010](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c392010.a) | 3.9.2/7.3 | Executable PASS | covered probe; P6f object orientation |
| [C392011](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c392011.a) | 3.9.2/7.3 | Executable PASS | covered probe; P6f object orientation |
| [C392D01](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c392d01.a); [f392d00.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/support/f392d00.a) | 3.9.2/7.3 | Executable PASS | covered probe; P6f object orientation |
| [C392D02](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c392d02.a); [f392d00.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/support/f392d00.a) | 3.9.2/7.3 | Executable PASS | covered probe; P6f object orientation |
| [C393001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c393001.a) | 3.9.3 | Executable PASS | covered probe; P6f object orientation |
| [C393012](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c393012.a) | 3.9.3 | Executable PASS | covered probe; P6f object orientation |
| [C393013](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c393013.a) | 3.9.3/12.5.1 | Executable PASS | repaired; P6g derived formals and private-override public views |
| [C393A02](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c393a02.a); [f393a00.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/support/f393a00.a) | 3.9.3 | Executable PASS | covered probe; P6f object orientation |
| [C393A03](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c393a03.a); [f393a00.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/support/f393a00.a) | 3.9.3 | Executable PASS | covered probe; P6f object orientation |
| [C393A06](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c393a06.a); [f393a00.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/support/f393a00.a) | 3.9.3 | Executable PASS | covered probe; P6f object orientation |
| [C394002](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c394002.a) | 3.9.4 | Executable PASS | covered probe; P6f object orientation |
| [C394003](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c394003.a) | 3.9.4 | Executable PASS | covered probe; P6f object orientation |
| [C3A0001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0001.a) | 3.10 | Executable PASS | covered probe; P6d |
| [C3A0002](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0002.a) | 3.10 | Executable PASS | covered probe; P6d |
| [C3A0003](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0003.a) | 3.10 | Executable PASS | covered probe; P6d |
| [C3A0004](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0004.a) | 3.10 | Executable PASS | covered probe; P6d |
| [C3A0005](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0005.a) | 3.10 | Executable PASS | covered probe; P6d |
| [C3A0007](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0007.a) | 3.10 | Executable PASS | repaired; P6f tagged/private views |
| [C3A0008](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0008.a) | 3.10 | Executable PASS | covered probe; P6d |
| [C41103A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41103a.ada) | 4.1.1–4.1.2 | Executable PASS | covered probe; P6f-prelude |
| [C41104A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41104a.ada) | 4.1.1–4.1.2 | Support/harness blocked | support adapter gap; P8 REPORT adapter |
| [C41105A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41105a.ada) | 4.1.1–4.1.2 | Executable PASS | covered probe; P6f-prelude |
| [C41107A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41107a.ada) | 4.1.1–4.1.2 | Executable PASS | covered probe; P3/P6b |
| [C41204A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41204a.ada) | 4.1.1–4.1.2 | Executable PASS | covered probe; P3/P6b |
| [C41205A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41205a.ada) | 4.1.1–4.1.2 | Executable PASS | covered probe; P3/P6b |
| [C41206A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41206a.ada) | 4.1.1–4.1.2 | Executable PASS | covered probe; P3/P6b |
| [C41207A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41207a.ada) | 4.1.1–4.1.2 | Legal C-test rejected | documented language gap; P8 discrete-range syntax audit |
| [C432001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c432001.a) | 4.3 | Executable PASS | repaired; P6f tagged/private views |
| [C432004](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c432004.a) | 4.3 | Executable PASS | repaired; P6f tagged/private views |
| [C432005](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c432005.a) | 4.3 | Legal C-test rejected | cross feature language gap; P9 full tasking |
| [C43204A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c43204a.ada) | 4.3 | Legal C-test rejected | documented language gap; P8 discrete-range syntax audit |
| [C460004](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c460004.a) | 4.6/4.9 | Executable PASS | covered probe; P3/P5/P6d |
| [C460005](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c460005.a) | 4.6/4.9 | Executable PASS | covered probe; P3/P5/P6d |
| [C460006](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c460006.a) | 4.6/4.9 | Executable PASS | abstract operations and tagged view conversions; P6f |
| [C49020A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c49020a.ada) | 4.6/4.9 | Executable PASS | covered probe; P3/P5/P6d |
| [C49021A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c49021a.ada) | 4.6/4.9 | Executable PASS | covered probe; P3/P5/P6d |
| [C49022B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c49022b.ada) | 4.6/4.9 | Executable PASS | covered probe; P3/P5/P6d |
| [C49022C](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c49022c.ada) | 4.6/4.9 | Executable PASS | covered probe; P6f-prelude |
| [C540001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c5/c540001.a) | 5.4 | Legal C-test rejected | expanded character-literal names (`Pkg.'A'`); P6g enumeration literals |
| [C540002](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c5/c540002.a) | 5.4 | Executable PASS | discrete-context selector resolution repaired; P6f-prelude |
| [C54A03A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c5/c54a03a.ada) | 5.4 | Legal C-test rejected | predefined library gap; P7 predefined ASCII |
| [C54A07A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c5/c54a07a.ada) | 5.4 | Support/harness blocked | support adapter gap; P8 REPORT adapter |
| [C640001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c640001.a) | 6.4–6.5 | Executable PASS | repaired; P6g tagged private formals |
| [C64002B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c64002b.ada) | 6.4–6.5 | Executable PASS | covered probe; P3/P6e |
| [C64004G](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c64004g.ada) | 6.4–6.5 | Executable PASS | enclosing-name qualification repaired; P6f-prelude |
| [C64005A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c64005a.ada) | 6.4–6.5 | Executable PASS | enclosing-name qualification repaired; P6f-prelude |
| [C64005B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c64005b.ada) | 6.4–6.5 | Executable PASS | enclosing-name qualification repaired; P6f-prelude |
| [C64005C](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c64005c.ada) | 6.4–6.5 | Support/harness blocked | support adapter gap; P8 REPORT adapter; P7 predefined ASCII |
| [C65003A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c65003a.ada) | 6.4–6.5 | Executable PASS | covered probe; P3/P6e |
| [C65003B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c65003b.ada) | 6.4–6.5 | Executable PASS | covered probe; P3/P6e |
| [C730004](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c730004.a) | 7.3/7.6 | Executable PASS | covered probe; P6f-prelude |
| [C73002A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c73002a.ada) | 7.3/7.6 | Executable PASS | covered probe; P6f-prelude |
| [C760001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c760001.a) | 7.3/7.6 | Executable PASS | covered probe; P3/P6a/P6c/P6e |
| [C760002](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c760002.a) | 7.3/7.6 | Executable PASS | covered probe; P3/P6a/P6c/P6e |
| [C760009](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c760009.a) | 7.3/7.6 | Executable PASS | repaired; P6f tagged/private views |
| [C760010](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c760010.a) | 7.3/7.6 | Executable PASS | repaired; P6g tagged private formals |
| [C760011](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c760011.a) | 7.3/7.6 | Support/harness blocked | support gap; P8 REPORT.Ident_Char adapter |
| [C760013](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c760013.a) | 7.3/7.6 | Executable PASS | covered probe; P3/P6a/P6c/P6e |
| [C761001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c761001.a) | 7.3/7.6 | Legal C-test rejected | cross feature language gap; P9 task entries |
| [C761004](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c761004.a) | 7.3/7.6 | Support/harness blocked | support adapter gap; P8 TCTouch adapter |
| [C761006](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c761006.a) | 7.3/7.6 | Executable PASS | controlled assignment/adjustment repair; P6f-prelude |
| [C761010](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c761010.a) | 7.3/7.6 | Legal C-test rejected | documented language gap; P6d/P6g access subtypes; dependent controlled arrays |
| [C83007A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c83007a.ada) | 8.3–8.5 | Executable PASS | covered probe; P3/P6e |
| [C83022A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c83022a.ada) | 8.3–8.5 | Executable PASS | enclosing-name qualification repaired; P6f-prelude |
| [C83024A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c83024a.ada) | 8.3–8.5 | Executable PASS | enclosing-name qualification repaired; P6f-prelude |
| [C840001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c840001.a) | 8.3–8.5 | Executable PASS | operator visibility/expanded-call repair; P6f-prelude |
| [C84002A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c84002a.ada) | 8.3–8.5 | Executable PASS | covered probe; P3/P6e |
| [C84005A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c84005a.ada) | 8.3–8.5 | Executable PASS | covered probe; P3/P6e |
| [C84008A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c84008a.ada) | 8.3–8.5 | Executable PASS | operator visibility/expanded-call repair; P6f-prelude |
| [C84009A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c84009a.ada) | 8.3–8.5 | Executable PASS | operator visibility/expanded-call repair; P6f-prelude |
| [C85005A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c85005a.ada) | 8.3–8.5 | Legal C-test rejected | cross feature language gap; P9 full tasking |
| [C85005B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c85005b.ada) | 8.3–8.5 | Legal C-test rejected | cross feature language gap; P9 full tasking |
| [C85005F](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c85005f.ada) | 8.3–8.5 | Executable PASS | covered probe; P3/P6e |
| [CA1003A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/ca/ca1003a.ada) | 10.1 | Executable PASS | covered probe; P3/P6e |
| [CA1004A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/ca/ca1004a.ada) | 10.1 | Executable PASS | covered probe; P3/P6e |
| [CA1005A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/ca/ca1005a.ada) | 10.1 | Executable PASS | covered probe; P3/P6e |
| [C940005](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c9/c940005.a) | 9.4–9.5 | Support/harness blocked | support adapter gap; P8 ImpDef configuration; P9 protected/tasking semantics |
| [C940010](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c9/c940010.a) | 9.4–9.5 | Support/harness blocked | support adapter gap; P8 ImpDef configuration; P9 protected/tasking semantics |
| [B330001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b330001.a) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B33101A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b33101a.ada) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B33102A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b33102a.ada) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B34001B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b34001b.ada) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B34002B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b34002b.ada) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B36001A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b36001a.ada) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B36002A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b36002a.ada) | 3/4/6/8 | B rejected, ungraded | mixed-index diagnostics repaired in P6f-prelude; grading: P8 B-test harness |
| [B37004A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b37004a.ada) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B394A01](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b394a01.a) | 3.9.4/12.5.1/12.5.5 | B rejected, ungraded | progenitor-type legality, including formals; P8 grading/P9 concurrent syntax |
| [B394A02](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b394a02.a) | 3.9.4/12.5.1/12.5.5 | B rejected, ungraded | nonlimited-interface descendant legality, including formals; P8 grading/P9 concurrent syntax |
| [B3A0001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b3a0001.a) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B3A0002](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b3a0002.a) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B43002D](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b4/b43002d.ada) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B43002E](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b4/b43002e.ada) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B43002F](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b4/b43002f.ada) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B460001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b4/b460001.a) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B64002A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b6/b64002a.ada) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B64002C](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b6/b64002c.ada) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B64003A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b6/b64003a.ada) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B650002](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b6/b650002.a) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B650004](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b6/b650004.a) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B840001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b8/b840001.a) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B394A03](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b394a03.a) | 3.9.4/12.5.1/12.5.5 | B rejected, ungraded | Task ancestry/conflicting task-protected ancestors; P8 grading/P9 execution |
| [B394A04](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b394a04.a) | 3.9.4/12.5.1/12.5.5 | B rejected, ungraded | Protected ancestry; P8 grading/P9 execution |
| [B394A05](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b394a05.a) | 3.9.4/12.5.1/12.5.5 | B rejected, ungraded | Synchronized ancestry; P8 grading/P9 execution |
