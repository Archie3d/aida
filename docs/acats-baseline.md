# ACATS completed-feature baseline — 2026-10-25

This backfill adds **106 unchanged upstream cases**: 86 C-tests and 20 B-test
probes, alongside the existing 55-case selection. The recorded host is Darwin
arm64. The compiler build hash, exact observations and source provenance are in
[the snapshot](../tests/acats/baseline/expected.json) and
[selection manifest](../tests/acats/baseline/manifest.json).
This is a baseline of the current subset, not a conformance claim.

| Observation | Cases |
| --- | ---: |
| B accepted, ungraded | 1 |
| B rejected, ungraded | 19 |
| Legal C-test rejected | 37 |
| Support/harness blocked | 7 |
| Executable PASS | 37 |
| Executable failure | 5 |

No crashes or timeouts were observed. **42 executable C-cases fail** (37 compile
rejections and five execution failures); seven further C-cases are blocked by
support adapters. The 19 rejected B-probes are ungraded, not 19 legality passes.
The accepted B36002A probe warrants investigation of mixed constrained/
unconstrained array-index legality.

## Executable failures to investigate first

| Case | Observation | Area |
| --- | --- | --- |
| C34001A | Unhandled Constraint_Error in derived enumeration operations; localize before changing implementation. | P3/P5 |
| C34001D | Unhandled Constraint_Error in derived Boolean operations; localize before changing implementation. | P3/P5 |
| C34002A | Unhandled Constraint_Error in derived integer operations; localize before changing implementation. | P3/P5 |
| C761006 | REPORT: other adjustment not performed. Audit adjustment exception handling and required remaining adjustments. | P3/P6a/P6c/P6e |
| C84009A | REPORT: BINARY "-" ALREADY VISIBLE FOR TYPE INT. Audit primitive operator/use visibility. | P3/P6e |

These results establish reproduction cases. They do not yet establish the
precise root cause, and no compiler behavior was changed during this baseline
capture. Legal-source rejections also need review: some hit explicit later
roadmap features, while others expose missing behavior in completed areas.

## Coverage by area

Counts are observations, not percentages of RM coverage. Compound cases may
be blocked before reaching the feature named in their objective.

| Area | C PASS | C rejected | C runtime failure | Harness blocked | B ungraded |
| --- | ---: | ---: | ---: | ---: | ---: |
| derived scalars | 5 | 1 | 3 | 1 | 0 |
| arrays discriminants | 4 | 6 | 0 | 0 | 0 |
| access callbacks | 6 | 1 | 0 | 0 | 0 |
| indexing slices | 4 | 3 | 0 | 1 | 0 |
| aggregates | 0 | 4 | 0 | 0 | 0 |
| conversions static | 5 | 2 | 0 | 0 | 0 |
| control flow | 0 | 3 | 0 | 1 | 0 |
| calls returns | 3 | 4 | 0 | 1 | 0 |
| private finalization | 3 | 7 | 1 | 1 | 0 |
| visibility renaming | 4 | 6 | 1 | 0 | 0 |
| loading subunits | 3 | 0 | 0 | 0 | 0 |
| protected objects | 0 | 0 | 0 | 2 | 0 |
| legality sample | 0 | 0 | 0 | 0 | 20 |

## Follow-up order

1. Localize the five executable failures and the accepted B36002A probe; turn
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
crash/timeout handling and source/snapshot integrity. Only the 37 observed
executable PASS cases join the normal ACATS test label. The full cohort has a
separate snapshot-stability target; matching a recorded failure is not an
ACATS pass. [Commands and grading boundaries](../tests/acats/baseline/README.md).

## Case inventory

Each objective, source hash and owning feature is recorded in the manifest.
The table below gives the observed status and initial triage for every case.
“Front-end rejection to investigate” is not a confirmed root-cause diagnosis.

| Case / upstream source | RM area | Observation | Initial triage / next owner |
| --- | --- | --- | --- |
| [C34001A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c34001a.ada) | 3.4–3.5 | Executable failure | execution failure; P3/P5/P8 |
| [C34001C](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c34001c.ada) | 3.4–3.5 | Executable PASS | covered probe; P3/P5 |
| [C34001D](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c34001d.ada) | 3.4–3.5 | Executable failure | execution failure; P3/P5/P8 |
| [C34002A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c34002a.ada) | 3.4–3.5 | Executable failure | execution failure; P3/P5/P8 |
| [C34002C](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c34002c.ada) | 3.4–3.5 | Executable PASS | covered probe; P3/P5 |
| [C35003A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c35003a.ada) | 3.4–3.5 | Legal C-test rejected | documented language gap; P8 range/subtype syntax audit |
| [C35003D](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c35003d.ada) | 3.4–3.5 | Executable PASS | covered probe; P3/P5 |
| [C35502A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c35502a.ada) | 3.4–3.5 | Executable PASS | covered probe; P3/P5 |
| [C35502C](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c35502c.ada) | 3.4–3.5 | Support/harness blocked | support adapter gap; P8 ACATS harness: REPORT.ident_char |
| [C35502G](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c35502g.ada) | 3.4–3.5 | Executable PASS | covered probe; P3/P5 |
| [C36104A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c36104a.ada) | 3.6–3.8 | Legal C-test rejected | documented language gap; P8 range/subtype syntax audit |
| [C36104B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c36104b.ada) | 3.6–3.8 | Legal C-test rejected | documented language gap; P8 range/subtype syntax audit |
| [C36180A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c36180a.ada) | 3.6–3.8 | Legal C-test rejected | documented language gap; P8 range/subtype syntax audit |
| [C37002A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37002a.ada) | 3.6–3.8 | Legal C-test rejected | documented language gap; P8 range/subtype syntax audit |
| [C37003A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37003a.ada) | 3.6–3.8 | Legal C-test rejected | documented language gap; P8 access-subtype audit |
| [C37003B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37003b.ada) | 3.6–3.8 | Executable PASS | covered probe; P3/P6b/P6c |
| [C37005A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37005a.ada) | 3.6–3.8 | Executable PASS | covered probe; P3/P6b/P6c |
| [C37006A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37006a.ada) | 3.6–3.8 | Legal C-test rejected | documented language gap; P8 access-subtype audit |
| [C37008A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37008a.ada) | 3.6–3.8 | Executable PASS | covered probe; P3/P6b/P6c |
| [C37008B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c37008b.ada) | 3.6–3.8 | Executable PASS | covered probe; P3/P6b/P6c |
| [C3A0001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0001.a) | 3.10 | Executable PASS | covered probe; P6d |
| [C3A0002](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0002.a) | 3.10 | Executable PASS | covered probe; P6d |
| [C3A0003](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0003.a) | 3.10 | Executable PASS | covered probe; P6d |
| [C3A0004](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0004.a) | 3.10 | Executable PASS | covered probe; P6d |
| [C3A0005](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0005.a) | 3.10 | Executable PASS | covered probe; P6d |
| [C3A0007](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0007.a) | 3.10 | Legal C-test rejected | documented language gap; P6f |
| [C3A0008](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0008.a) | 3.10 | Executable PASS | covered probe; P6d |
| [C41103A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41103a.ada) | 4.1.1–4.1.2 | Legal C-test rejected | frontend rejection to investigate; P3/P6b/P8 |
| [C41104A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41104a.ada) | 4.1.1–4.1.2 | Support/harness blocked | support adapter gap; P8 ACATS harness: REPORT.ident_char |
| [C41105A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41105a.ada) | 4.1.1–4.1.2 | Legal C-test rejected | frontend rejection to investigate; P3/P6b/P8 |
| [C41107A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41107a.ada) | 4.1.1–4.1.2 | Executable PASS | covered probe; P3/P6b |
| [C41204A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41204a.ada) | 4.1.1–4.1.2 | Executable PASS | covered probe; P3/P6b |
| [C41205A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41205a.ada) | 4.1.1–4.1.2 | Executable PASS | covered probe; P3/P6b |
| [C41206A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41206a.ada) | 4.1.1–4.1.2 | Executable PASS | covered probe; P3/P6b |
| [C41207A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c41207a.ada) | 4.1.1–4.1.2 | Legal C-test rejected | documented language gap; P8 range/subtype syntax audit |
| [C432001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c432001.a) | 4.3 | Legal C-test rejected | documented language gap; P6f |
| [C432004](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c432004.a) | 4.3 | Legal C-test rejected | documented language gap; P6f |
| [C432005](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c432005.a) | 4.3 | Legal C-test rejected | cross feature language gap; P9 |
| [C43204A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c43204a.ada) | 4.3 | Legal C-test rejected | documented language gap; P8 range/subtype syntax audit |
| [C460004](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c460004.a) | 4.6/4.9 | Executable PASS | covered probe; P3/P5/P6d |
| [C460005](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c460005.a) | 4.6/4.9 | Executable PASS | covered probe; P3/P5/P6d |
| [C460006](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c460006.a) | 4.6/4.9 | Legal C-test rejected | documented language gap; P6f |
| [C49020A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c49020a.ada) | 4.6/4.9 | Executable PASS | covered probe; P3/P5/P6d |
| [C49021A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c49021a.ada) | 4.6/4.9 | Executable PASS | covered probe; P3/P5/P6d |
| [C49022B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c49022b.ada) | 4.6/4.9 | Executable PASS | covered probe; P3/P5/P6d |
| [C49022C](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c49022c.ada) | 4.6/4.9 | Legal C-test rejected | frontend rejection to investigate; P3/P5/P6d/P8 |
| [C540001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c5/c540001.a) | 5.4 | Legal C-test rejected | documented language gap; P6g |
| [C540002](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c5/c540002.a) | 5.4 | Legal C-test rejected | frontend rejection to investigate; P3/P6e/P8 |
| [C54A03A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c5/c54a03a.ada) | 5.4 | Legal C-test rejected | predefined library gap; P7 predefined library |
| [C54A07A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c5/c54a07a.ada) | 5.4 | Support/harness blocked | support adapter gap; P8 ACATS harness: REPORT.ident_char |
| [C640001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c640001.a) | 6.4–6.5 | Legal C-test rejected | documented language gap; P6g |
| [C64002B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c64002b.ada) | 6.4–6.5 | Executable PASS | covered probe; P3/P6e |
| [C64004G](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c64004g.ada) | 6.4–6.5 | Legal C-test rejected | frontend rejection to investigate; P3/P6e/P8 |
| [C64005A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c64005a.ada) | 6.4–6.5 | Legal C-test rejected | frontend rejection to investigate; P3/P6e/P8 |
| [C64005B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c64005b.ada) | 6.4–6.5 | Legal C-test rejected | frontend rejection to investigate; P3/P6e/P8 |
| [C64005C](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c64005c.ada) | 6.4–6.5 | Support/harness blocked | support adapter gap; P8 ACATS harness: REPORT.ident_char |
| [C65003A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c65003a.ada) | 6.4–6.5 | Executable PASS | covered probe; P3/P6e |
| [C65003B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c65003b.ada) | 6.4–6.5 | Executable PASS | covered probe; P3/P6e |
| [C730004](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c730004.a) | 7.3/7.6 | Legal C-test rejected | frontend rejection to investigate; P3/P6a/P6c/P6e/P8 |
| [C73002A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c73002a.ada) | 7.3/7.6 | Legal C-test rejected | frontend rejection to investigate; P3/P6a/P6c/P6e/P8 |
| [C760001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c760001.a) | 7.3/7.6 | Executable PASS | covered probe; P3/P6a/P6c/P6e |
| [C760002](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c760002.a) | 7.3/7.6 | Executable PASS | covered probe; P3/P6a/P6c/P6e |
| [C760009](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c760009.a) | 7.3/7.6 | Legal C-test rejected | documented language gap; P6f |
| [C760010](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c760010.a) | 7.3/7.6 | Legal C-test rejected | documented language gap; P6g |
| [C760011](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c760011.a) | 7.3/7.6 | Legal C-test rejected | frontend rejection to investigate; P3/P6e/P8: REPORT.ident_char |
| [C760013](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c760013.a) | 7.3/7.6 | Executable PASS | covered probe; P3/P6a/P6c/P6e |
| [C761001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c761001.a) | 7.3/7.6 | Legal C-test rejected | cross feature language gap; P9 |
| [C761004](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c761004.a) | 7.3/7.6 | Support/harness blocked | support adapter gap; P8 ACATS harness: TCTouch.Flush, TCTouch.Validate third argument |
| [C761006](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c761006.a) | 7.3/7.6 | Executable failure | execution failure; P3/P6a/P6c/P6e/P8 |
| [C761010](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c7/c761010.a) | 7.3/7.6 | Legal C-test rejected | documented language gap; P6f |
| [C83007A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c83007a.ada) | 8.3–8.5 | Executable PASS | covered probe; P3/P6e |
| [C83022A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c83022a.ada) | 8.3–8.5 | Legal C-test rejected | frontend rejection to investigate; P3/P6e/P8 |
| [C83024A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c83024a.ada) | 8.3–8.5 | Legal C-test rejected | frontend rejection to investigate; P3/P6e/P8 |
| [C840001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c840001.a) | 8.3–8.5 | Legal C-test rejected | frontend rejection to investigate; P3/P6e/P8 |
| [C84002A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c84002a.ada) | 8.3–8.5 | Executable PASS | covered probe; P3/P6e |
| [C84005A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c84005a.ada) | 8.3–8.5 | Executable PASS | covered probe; P3/P6e |
| [C84008A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c84008a.ada) | 8.3–8.5 | Legal C-test rejected | frontend rejection to investigate; P3/P6e/P8 |
| [C84009A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c84009a.ada) | 8.3–8.5 | Executable failure | execution failure; P3/P6e/P8 |
| [C85005A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c85005a.ada) | 8.3–8.5 | Legal C-test rejected | cross feature language gap; P9 |
| [C85005B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c85005b.ada) | 8.3–8.5 | Legal C-test rejected | cross feature language gap; P9 |
| [C85005F](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c8/c85005f.ada) | 8.3–8.5 | Executable PASS | covered probe; P3/P6e |
| [CA1003A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/ca/ca1003a.ada) | 10.1 | Executable PASS | covered probe; P3/P6e |
| [CA1004A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/ca/ca1004a.ada) | 10.1 | Executable PASS | covered probe; P3/P6e |
| [CA1005A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/ca/ca1005a.ada) | 10.1 | Executable PASS | covered probe; P3/P6e |
| [C940005](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c9/c940005.a) | 9.4–9.5 | Support/harness blocked | support adapter gap; P8 ACATS harness: ImpDef requires target-specific support configuration |
| [C940010](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c9/c940010.a) | 9.4–9.5 | Support/harness blocked | support adapter gap; P8 ACATS harness: ImpDef requires target-specific support configuration |
| [B330001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b330001.a) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B33101A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b33101a.ada) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B33102A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b33102a.ada) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B34001B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b34001b.ada) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B34002B](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b34002b.ada) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B36001A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b36001a.ada) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
| [B36002A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b36002a.ada) | 3/4/6/8 | B accepted, ungraded | ungraded legality probe; P8 B-test harness |
| [B37004A](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/b3/b37004a.ada) | 3/4/6/8 | B rejected, ungraded | ungraded legality probe; P8 B-test harness |
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
