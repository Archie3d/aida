# ACATS completed-feature baseline — refreshed 2026-11-11

This backfill adds **161 unchanged upstream cases**: 135 C-tests and 26 B-test
probes, alongside the strict focused selection. The recorded host is Darwin
arm64. The compiler build hash, exact observations and source provenance are in
[the snapshot](../tests/acats/baseline/expected.json) and
[selection manifest](../tests/acats/baseline/manifest.json).
This is a baseline of the current subset, not a conformance claim.

| Observation | Cases |
| --- | ---: |
| B accepted, ungraded | 0 |
| B rejected, ungraded | 26 |
| Legal C-test rejected | 5 |
| Support/harness blocked | 0 |
| Executable PASS | 130 |
| Executable failure | 0 |

No crashes or timeouts were observed. Five legal C-tests are rejected.
C35502C now completes its unchanged REPORT protocol after enumeration Value
was corrected to trim surrounding spaces only; tabs raise Constraint_Error.
C761001 now passes unchanged after task entry declarations were supported;
its unused entry checks library-task termination before controlled finalization.
It does not exercise rendezvous or select. Task-interface rejections remain,
with updated wording. B650004 now grades PASS after the Phase 12 nested-body
return repair; B650002 also grades PASS after its Phase 12 return-form diagnostic review.

The raw B observations stay ungraded for historical comparison. The independent
[location/group-aware grader](acats-grading.md) records **26 B PASS (25 original foundations, one local foundation adapter)**,
with separate legal controls. Only complete B passes join the regular ACATS label.

## Non-tasking legality increment (Phase 8)

Unchanged B33101A/B34001B/B34002B/B37004A/B460001/BC60001 now grade PASS.
The [audit](legality-audit.md) distinguishes compiler repairs from diagnostic
and location review. The raw cohort still contains 26 rejected B observations;
only these six diagnostic streams change, with all C outcomes unchanged and all
149 upstream hashes intact. The independent grading report has 17 PASS and
nine FAIL; the nine blocked full sources retain P9 prerequisite owners.

## Support-protocol increment (Phase 8)

C761004 now passes unchanged after TCTouch Flush/Validate support. C940005 and
C940010 pass unchanged with local ImpDef task-progress delays. The remaining
142 raw observations and all 149 upstream hashes are unchanged. The detailed
[grading contract, failed cases and support boundaries](acats-grading.md) distinguish
harness completion from the language defects and unsupported prerequisites
exposed by 15 failed B-tests. Older increment counts below describe their
historical snapshots.

## Current-subset adoption audit (Phase 8)

The [bounded adoption review](acats-adoption.md) adds 16 sources selected before
probing: seven Size/mapping/generic cases, five Ada 2005 null-exclusion cases and
four formal-subprogram matching/default/literal cases. Fifteen execute unchanged
as PASS. CC3605A stops at line 248's access-to-array subtype constraint, the
existing limitation also recorded for C761010; it remains a baseline-only exact
rejection observation. No generic-profile conformance pass is inferred from its
partial source. The original 129 observations are unchanged, and the expanded
145-case snapshot validates all 149 source hashes. No compiler/runtime or
support-adapter changes were made.

## Access-subtype record constraints (Phase 8)

C37003A and C37006A now execute unchanged as PASS. Designated-record
constraints reuse captured discriminant values and check non-null access values
at value boundaries, including component initialization and conversion.
[Contract and local cross-unit/tasking coverage](access-subtype-constraints.md).
All other outcomes remain unchanged; B37004A has updated, ungraded diagnostics
where its previously unsupported access constraint is now analyzed.

## Discrete-range observations (Phase 8)

C35003A, C36104A/B, C36180A, C37002A, C41207A and C43204A now execute
unchanged as PASS. They cover range attributes, explicit subtype ranges,
null-range compatibility, declaration-time checks and call/qualified bounds.
C3A2003 passes separately in the focused selection. [Contract](discrete-ranges.md).

The complete 129-case refresh also observes the already available REPORT
adapter: C41104A, C54A07A and C760011 pass; C35502C/C64005C now reach missing
ASCII declarations. C940005/C940010 reach missing services in the partial
ImpDef unit, recorded as compile rejections. No adapter or predefined-library
implementation changed in this increment. B-probes remain ungraded.

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

## Null-default observation (Phase 6g)

BC60001 adds one unchanged Ada 2005 null-default legality source. Its frozen
B_REJECTED_UNGRADED observation contains the four illegal function/abstract
null-default syntax errors; parsing errors prevent semantic grading of the
remaining legal and intrinsic-convention cases. Local `genericnull*` isolates
those cases, including Access restrictions through aliases/forwarding. All 128
previous observations were unchanged; at that increment the cohort had 79 PASS,
16 compile rejections, eight harness blockers and 26 ungraded B observations.

## Reviewed repairs

Phase 6h retains all 129 observations unchanged. Its new strict executable
C651001 selection lives alongside this baseline and checks nonreturning
procedure completion. Assertion/check policies and their conservative check
retention are documented in the [contract](pragmas-and-checks.md).

Phase 6g promotes C393013, C640001 and C760010 to executable PASS, unchanged.
C393013 runs the complete null-extension controlling-result scenario through
generic and private extensions, including private overrides. C640001 exercises
tagged generic callbacks and redispatch. C760010 verifies controlled lifetimes
and explicit/implicit exception propagation through tagged private formals.
C393013 now uses `acats.backfill_c393013`; its frozen-rejection probe is removed.
C540001 now passes unchanged after enumeration-literal function actuals,
expanded character names (`Pkg.'A'`) and static related-enumeration conversions.
The complete case-expression scenario is registered as `acats.backfill_c540001`.
It is the only changed observation at this increment: all other 128 observations
remained unchanged; the cohort then had 80 executable passes.

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
C3A0009 and C761012 also pass unchanged; C3A0013 now executes as PASS
after the Phase 7 range-attribute array-declaration prerequisite. See the
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
also repaired; C37006A and C760011 then retained independent prerequisites.
Both now execute unchanged as PASS after their subsequent increments.
`supportedrejections`/`supportedrejectionerrors` cover legal and rejection
controls in whole/per-unit/deterministic modes, with four tasks and compiler
ASan/UBSan. All four new passes join the strict selection.

The [remaining-rejections audit](supported-rejections.md) records concrete
prerequisites. Removing a prerequisite still requires rerunning the unchanged
compound case before promotion.

## Coverage by area

Counts are observations, not percentages of RM coverage. Compound cases may
be blocked before reaching the feature named in their objective.

| Area | C PASS | C rejected | C runtime failure | Harness blocked | B raw rejected |
| --- | ---: | ---: | ---: | ---: | ---: |
| derived scalars | 9 | 0 | 1 | 0 | 0 |
| arrays discriminants | 10 | 0 | 0 | 0 | 0 |
| object orientation | 17 | 0 | 0 | 0 | 0 |
| access callbacks | 7 | 0 | 0 | 0 | 0 |
| indexing slices | 8 | 0 | 0 | 0 | 0 |
| aggregates | 3 | 1 | 0 | 0 | 0 |
| conversions static | 7 | 0 | 0 | 0 | 0 |
| control flow | 4 | 0 | 0 | 0 | 0 |
| calls returns | 8 | 0 | 0 | 0 | 0 |
| private finalization | 11 | 1 | 0 | 0 | 0 |
| visibility renaming | 9 | 2 | 0 | 0 | 0 |
| loading subunits | 3 | 0 | 0 | 0 | 0 |
| protected objects | 2 | 0 | 0 | 0 | 0 |
| legality sample | 0 | 0 | 0 | 0 | 21 |
| generic interfaces | 0 | 0 | 0 | 0 | 5 |
| representation | 7 | 0 | 0 | 0 | 0 |
| access null exclusions | 5 | 0 | 0 | 0 | 0 |
| generic subprograms | 3 | 1 | 0 | 0 | 0 |

## Follow-up order

1. Validate target configuration on other supported hosts. The [non-tasking
   B-test findings](legality-audit.md) now pass; concurrent prerequisites retain
   their Phase 9 owners.
2. Investigate the remaining five legal C rejections;
   turn confirmed defects into focused compiler regressions before fixing them.
3. Current TCTouch and local ImpDef gaps for the selected support-blocked probes
   are repaired; broader protocol/applicability work requires its own selection.
4. Retain explicit unsupported-feature boundaries and promote complete cases
   as features or defects are resolved.

The baseline runner tests enforce strict REPORT completion/identity, raw B
observation boundaries, crash/timeout handling and source/snapshot integrity.
Its 113 executable C passes and the independent grader's 17 complete B passes
join normal ACATS labels. Snapshot stability of a recorded failure is not an
ACATS pass. [Commands and grading boundaries](../tests/acats/baseline/README.md).

## Case inventory

Each objective, source hash and owning feature is recorded in the manifest.
The table below gives the raw observed status and triage for every case;
B-test grades are reported independently in the linked grading report.
“Front-end rejection to investigate” is not a confirmed root-cause diagnosis.

| Case / upstream source | RM area | Observation | Initial triage / next owner |
| --- | --- | --- | --- |
| [C34001A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c34001a.ada) | 3.4–3.5 | Executable PASS | scalar Value base-range repair; P6f-prelude |
| [C34001C](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c34001c.ada) | 3.4–3.5 | Executable PASS | covered probe; P3/P5 |
| [C34001D](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c34001d.ada) | 3.4–3.5 | Executable PASS | scalar Value base-range repair; P6f-prelude |
| [C34002A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c34002a.ada) | 3.4–3.5 | Executable PASS | scalar Value base-range repair; P6f-prelude |
| [C34002C](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c34002c.ada) | 3.4–3.5 | Executable PASS | covered probe; P3/P5 |
| [C35003A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c35003a.ada) | 3.4–3.5 | Executable PASS | covered probe; P8 discrete ranges |
| [C35003D](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c35003d.ada) | 3.4–3.5 | Executable PASS | covered probe; P3/P5 |
| [C35502A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c35502a.ada) | 3.4–3.5 | Executable PASS | covered probe; P3/P5 |
| [C35502C](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c35502c.ada) | 3.4–3.5 | Executable PASS | P12 enumeration Value rejects surrounding HT and accepts spaces; unchanged REPORT protocol |
| [C35502G](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c35502g.ada) | 3.4–3.5 | Executable PASS | covered probe; P3/P5 |
| [C36104A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c36104a.ada) | 3.6–3.8 | Executable PASS | covered probe; P8 discrete ranges |
| [C36104B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c36104b.ada) | 3.6–3.8 | Executable PASS | covered probe; P8 discrete ranges |
| [C36180A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c36180a.ada) | 3.6–3.8 | Executable PASS | covered probe; P8 discrete ranges |
| [C37002A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37002a.ada) | 3.6–3.8 | Executable PASS | covered probe; P8 discrete ranges |
| [C37003A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37003a.ada) | 3.6–3.8 | Executable PASS | covered probe; P8 access-subtype record constraints |
| [C37003B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37003b.ada) | 3.6–3.8 | Executable PASS | covered probe; P3/P6b/P6c |
| [C37005A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37005a.ada) | 3.6–3.8 | Executable PASS | covered probe; P3/P6b/P6c |
| [C37006A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37006a.ada) | 3.6–3.8 | Executable PASS | covered probe; P8 access-subtype record constraints |
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
| [C41104A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41104a.ada) | 4.1.1–4.1.2 | Executable PASS | covered probe; P8 observation refresh |
| [C41105A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41105a.ada) | 4.1.1–4.1.2 | Executable PASS | covered probe; P6f-prelude |
| [C41107A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41107a.ada) | 4.1.1–4.1.2 | Executable PASS | covered probe; P3/P6b |
| [C41204A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41204a.ada) | 4.1.1–4.1.2 | Executable PASS | covered probe; P3/P6b |
| [C41205A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41205a.ada) | 4.1.1–4.1.2 | Executable PASS | covered probe; P3/P6b |
| [C41206A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41206a.ada) | 4.1.1–4.1.2 | Executable PASS | covered probe; P3/P6b |
| [C41207A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41207a.ada) | 4.1.1–4.1.2 | Executable PASS | covered probe; P8 discrete ranges |
| [C432001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c432001.a) | 4.3 | Executable PASS | repaired; P6f tagged/private views |
| [C432004](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c432004.a) | 4.3 | Executable PASS | repaired; P6f tagged/private views |
| [C432005](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c432005.a) | 4.3 | Legal C-test rejected | cross feature language gap; P9 full tasking |
| [C43204A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c43204a.ada) | 4.3 | Executable PASS | covered probe; P8 discrete ranges |
| [C460004](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c460004.a) | 4.6/4.9 | Executable PASS | covered probe; P3/P5/P6d |
| [C460005](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c460005.a) | 4.6/4.9 | Executable PASS | covered probe; P3/P5/P6d |
| [C460006](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c460006.a) | 4.6/4.9 | Executable PASS | abstract operations and tagged view conversions; P6f |
| [C49020A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c49020a.ada) | 4.6/4.9 | Executable PASS | covered probe; P3/P5/P6d |
| [C49021A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c49021a.ada) | 4.6/4.9 | Executable PASS | covered probe; P3/P5/P6d |
| [C49022B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c49022b.ada) | 4.6/4.9 | Executable PASS | covered probe; P3/P5/P6d |
| [C49022C](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c49022c.ada) | 4.6/4.9 | Executable PASS | covered probe; P6f-prelude |
| [C540001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c5/c540001.a) | 5.4 | Executable PASS | complete case-expression scenario, including expanded character names and literal function actuals |
| [C540002](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c5/c540002.a) | 5.4 | Executable PASS | discrete-context selector resolution repaired; P6f-prelude |
| [C54A03A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c5/c54a03a.ada) | 5.4 | PASS | P8 Standard.ASCII; unchanged executable case control |
| [C54A07A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c5/c54a07a.ada) | 5.4 | Executable PASS | covered probe; P8 observation refresh |
| [C640001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c640001.a) | 6.4–6.5 | Executable PASS | repaired; P6g tagged private formals |
| [C64002B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c64002b.ada) | 6.4–6.5 | Executable PASS | covered probe; P3/P6e |
| [C64004G](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c64004g.ada) | 6.4–6.5 | Executable PASS | enclosing-name qualification repaired; P6f-prelude |
| [C64005A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c64005a.ada) | 6.4–6.5 | Executable PASS | enclosing-name qualification repaired; P6f-prelude |
| [C64005B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c64005b.ada) | 6.4–6.5 | Executable PASS | enclosing-name qualification repaired; P6f-prelude |
| [C64005C](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c64005c.ada) | 6.4–6.5 | PASS | P8 Standard.ASCII; unchanged recursive static-chain control |
| [C65003A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c65003a.ada) | 6.4–6.5 | Executable PASS | covered probe; P3/P6e |
| [C65003B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c65003b.ada) | 6.4–6.5 | Executable PASS | covered probe; P3/P6e |
| [C730004](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c730004.a) | 7.3/7.6 | Executable PASS | covered probe; P6f-prelude |
| [C73002A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c73002a.ada) | 7.3/7.6 | Executable PASS | covered probe; P6f-prelude |
| [C760001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c760001.a) | 7.3/7.6 | Executable PASS | covered probe; P3/P6a/P6c/P6e |
| [C760002](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c760002.a) | 7.3/7.6 | Executable PASS | covered probe; P3/P6a/P6c/P6e |
| [C760009](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c760009.a) | 7.3/7.6 | Executable PASS | repaired; P6f tagged/private views |
| [C760010](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c760010.a) | 7.3/7.6 | Executable PASS | repaired; P6g tagged private formals |
| [C760011](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c760011.a) | 7.3/7.6 | Executable PASS | covered probe; P8 observation refresh |
| [C760013](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c760013.a) | 7.3/7.6 | Executable PASS | covered probe; P3/P6a/P6c/P6e |
| [C761001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c761001.a) | 7.3/7.6 | Executable PASS | covered probe; P9 task entries |
| [C761004](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c761004.a) | 7.3/7.6 | Executable PASS | TCTouch protocol repaired; P8 |
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
| [C940005](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c9/c940005.a) | 9.4–9.5 | Executable PASS | local task-progress configuration; P8 |
| [C940010](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c9/c940010.a) | 9.4–9.5 | Executable PASS | local task-progress configuration; P8 |
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
| [CD2A22E](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/cd/cd2a22e.ada) | 13.2–13.4, 12.3, 13.9 | Executable PASS | covered probe; P8 bit storage |
| [CD2A22I](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/cd/cd2a22i.ada) | 13.2–13.4, 12.3, 13.9 | Executable PASS | covered probe; P8 bit storage |
| [lencheck.ada](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/support/lencheck.ada); [CD2A22J](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/cd/cd2a22j.ada) | 13.2–13.4, 12.3, 13.9 | Executable PASS | covered probe; P8 bit storage |
| [lencheck.ada](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/support/lencheck.ada); [CD2A24A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/cd/cd2a24a.ada) | 13.2–13.4, 12.3, 13.9 | Executable PASS | covered probe; P8 bit storage |
| [CD2A24E](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/cd/cd2a24e.ada) | 13.2–13.4, 12.3, 13.9 | Executable PASS | covered probe; P8 bit storage |
| [CD2A24I](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/cd/cd2a24i.ada) | 13.2–13.4, 12.3, 13.9 | Executable PASS | covered probe; P8 bit storage |
| [CD2A24J](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/cd/cd2a24j.ada) | 13.2–13.4, 12.3, 13.9 | Executable PASS | covered probe; P8 bit storage |
| [C3A0019](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0019.a) | 3.10, 6.1, 12.4 | Executable PASS | covered probe; P6d/P6g null exclusions |
| [C3A0021](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0021.a) | 3.10, 6.1, 12.4 | Executable PASS | covered probe; P6d/P6g null exclusions |
| [C3A0022](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0022.a) | 3.10, 6.1, 12.4 | Executable PASS | covered probe; P6d/P6g null exclusions |
| [C3A0023](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0023.a) | 3.10, 6.1, 12.4 | Executable PASS | covered probe; P6d/P6g null exclusions |
| [C3A0024](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0024.a) | 3.10, 6.1, 12.4 | Executable PASS | covered probe; P6d/P6g null exclusions |
| [CC3603A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/cc/cc3603a.ada) | 12.6, 6.3.1 | Executable PASS | covered probe; P6g formal subprograms |
| [CC3605A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/cc/cc3605a.ada) | 12.6, 6.3.1 | Legal C-test rejected | existing access-to-array subtype constraint limit; P8 |
| [CC3606A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/cc/cc3606a.ada) | 12.6, 6.3.1 | Executable PASS | covered probe; P6g formal subprograms |
| [CC3606B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/cc/cc3606b.ada) | 12.6, 6.3.1 | Executable PASS | covered probe; P6g formal subprograms |

### Standard/System follow-up

The ASCII prerequisite is implemented. Unchanged C54A03A and C64005C now pass
and join the regular ACATS selection. C35502C now compiles and executes but
fails its leading/trailing HT controls: enumeration Value currently trims tabs
as well as spaces. This is a P3/P8 scalar-attribute correctness gap and is
recorded as RUN_FAILED. The other 142 outcomes are unchanged: the cohort now
has 109 PASS, eight legal-source rejections, one runtime failure, one support
blocker and 26 ungraded B rejections. Source bytes/hashes are unchanged.

Phase 9 selective accept enables unchanged C85005A and C85005B. Both complete
with their exact REPORT success protocol and join the strict backfill selection.
Their objectives concern object/formal renaming and parameter aliasing; their
helper tasks now execute real selective rendezvous. These passes do not cover
entry callable renaming, concurrent interfaces, requeue or source abort. The
snapshot now records 115 C passes, three C compilation rejections, one C runtime
failure and 26 ungraded B rejections; B conformance grades are unchanged.

## Phase 9 requeue/abort audit

A complete 145-case observation retains 115 executable C passes, three legal C
rejections, one C runtime failure and 26 raw ungraded B rejections. The stale
C432005/B650002/B650004 snapshots now reflect entry/accept parsing from the
preceding milestone: C432005 reaches the existing concurrent-component layout
limits, and both B tests reach return-legality checks. The independent B grader
is unchanged at 17 PASS and nine FAIL; neither return test is promoted. No
upstream source or grading policy was changed, and these results are not general
requeue/abort coverage. Local source/native controls supply that evidence.

## Concurrent discriminants and callable views (Phase 9)

Eight cases selected before probing now execute unchanged with complete REPORT
success: C910001/C940012/C940015 (discriminants and dependent private payloads),
C85014B/C85018A/C85018B (entry callable views) and C3A0018/C3A0027 (protected
callbacks/null exclusions). Per-case language reviews and original-byte hashes
are in the manifest; all 157 upstream files validate. [Implementation contract](concurrent-callable-views.md).
The previous 145 outcome classifications remain unchanged: the expanded total
is 123 C PASS, three legal-source rejections, one runtime failure and 26 raw B
rejections. B330001/B3A0002 now reach semantic diagnostics after their parser
blockers are removed. Their exact observations and independent grade snapshots
are refreshed without changing grading policies; both still FAIL. B grades
remain 17 PASS / nine FAIL. Earlier sections retain their historical counts.

## Concurrent interface increment (Phase 9)

C940017 was selected before probing and now passes unchanged with the complete
REPORT/TCTouch protocol. Its protected-interface procedures, functions and
entries dispatch across library units. Despite its 2019 creation date, its
source uses Ada 2005 syntax. C950001/C954027 were assessed and excluded because
they use the Ada 2012 Synchronization aspect; their hashes/reasons remain in the
manifest. All 158 vendored file hashes validate. [Contract](concurrent-interfaces.md).

The other 153 outcome classifications remain unchanged: 124 executable C passes,
three legal-source rejections, one runtime failure and 26 raw B rejections.
B394A01–B394A05 now reach ancestry/private-completion semantics. Exact raw and
grade snapshots were reviewed under unchanged policies; all five still grade
FAIL. The independent B total remains 17 PASS / nine FAIL. Rejection alone is
not a legality pass; earlier sections retain their historical counts.

## Task-services increment (Phase 9)

Six Ada 95/2005 sources were selected before probing. CXC7003, CXDA001,
CXDA002 and CXD1001 pass unchanged: controlled attribute Set_Value/Reinitialize,
suspension state and wakeup/duplicate-waiter behavior, and dynamic base priorities.
CXDA002 needs the documented local Impdef.Minimum_Task_Switch allowance of
0.1 seconds. No upstream source or REPORT grading was changed.

CXC7002 remains rejected because it requires Pack on a signed-integer array;
CXDA003 requires Locking_Policy and Interrupt_Priority, which remain explicit
unsupported constructs. These are retained as failures. All 164 vendored hashes
validate. The previous 154 outcome classifications remain unchanged; the new
selection has 128 executable passes, five legal-source rejections, one executable
failure and 26 ungraded B rejections. The independent B policy remains unchanged.
[Task-services contract](task-services.md).

## Task-termination increment (Phase 9)

Unchanged CXC7006 was selected before probing. Its first five-second execution
observation timed out because the source's sequential delay groups exceed six
seconds. A bounded 15-second per-case deadline exposed an escaping Tasking_Error
in Test_6: activation occurred before the activator installed its handlers.
Moving activation into the handled sequence fixes that defect. The full source
now passes normal, abnormal, unhandled, fallback/specific, local-finalization and
activation-failure tests with the strict REPORT protocol. No source or grading
rule was changed. Its AI05-0202 check is an Ada 2005 binding interpretation.

The selection now has 161 cases and 165 hash-verified sources, with 129 executable
passes, five compile rejections, one runtime failure and 26 ungraded B rejections.
Earlier sections retain historical counts. [Contract](task-termination.md).
