# Selected ACATS B-test grading and support

The Phase 8 harness grades all 26 pinned B-tests independently of the raw
[145-case baseline](acats-baseline.md). It currently records **11 PASS and
15 FAIL**, not 26 passes for nonzero compiler exits. This is a conservative,
local grader for an explicit selection, not the official ACATS grading tool
or a conformance assessment.

## Grading contract

[Policies](../tests/acats/grading/policies.json) name each case's objective,
permitted semantic diagnostic families, legal counterpart and any reviewed
location/recovery exceptions. The unchanged upstream `ERROR:` comments identify
required sites; `OPTIONAL ERROR:` sites may be undiagnosed. Locations are exact
source identities and line numbers. The multiline legacy B36002A declarations
explicitly allow either declaration line. Upstream brace/column annotations are
not interpreted; future location extensions require policy review. A diagnostic
in a helper, a different file, or an unmarked line is unexpected, including
errors on `OK` controls.

Every required site needs a matching diagnostic from its allowed group.
Recovery diagnostics need explicit file/line/message permissions and cannot
satisfy a required site. Currently only B43002F's secondary empty-association
message has such permission. Unsupported-feature diagnostics never satisfy a
site, even if a pattern would match them. Missing diagnostics, changed diagnostic
families, parser cascades, unrelated errors, compiler success, build failures,
crashes, timeouts and output-limit failures cannot pass a negative test.

Each grade also compiles the shared, separately authored
[legal counterpart](../tests/acats/grading/legal.adb). It checks distinct names,
valid derived conversions, discrete/constrained/unconstrained array indices,
record/array aggregates, writable aliased access, positional/named/defaulted
calls, scalar attributes and null procedure defaults. It supplements the legal
controls embedded in the unchanged B-sources; it does not establish unsupported
concurrent-interface legality. A legal-control rejection or unexpected output
fails the grade. Tests are frontend-only and do not execute B-programs.

The [reviewed report](../tests/acats/grading/expected.json) retains every missing
site and unexpected diagnostic. Only these complete observed passes are on the
regular `acats;b-tests` CTest labels:

B33102A, B36001A, B36002A, B3A0001, B43002D, B43002E, B43002F,
B64002A, B64002C, B64003A and B840001.

The other 15 cases stay failed and belong to their existing language owners:

| Cases | Observation / existing owner |
| --- | --- |
| B330001 | Legal task/protected discriminants block analysis; P9, indefinite-object/component legality P3/P6/P8 |
| B33101A | Six required diagnostics absent; final derived-type restriction is unrelated rejection; P3/P8 discriminant legality |
| B34001B/B34002B | Unsupported legacy attributes and incomplete diagnostic coverage; P3/P8 attribute audit |
| B37004A | Missing component/discriminant legality diagnostics and recovery outside allowed groups; P3/P6/P8 |
| B3A0002 | Named protected-subprogram access prerequisite; P9 |
| B460001 | Access-discriminant accessibility diagnostics absent at lines 238 and 259; P6d/P8 |
| B650002/B650004 | Task entry syntax blocks analysis of return legality; P9, P6e/P8 returns |
| B394A01–B394A05 | Concurrent-interface syntax blocks the tested ancestry rules; P9 |
| BC60001 | Intrinsic null-default subprogram Access diagnostic absent at line 102; P6g/P8 |

These findings are not fixed or waived by completing the harness item.
TODO.md explicitly tracks the non-tasking follow-up before host validation. Widening
a compiler feature requires reviewing its policy and re-running the full case.
The frozen raw B observations remain ungraded for historical comparison.

## Support adapters and re-probes

`REPORT.Ident_Char` was already implemented. An executable control checks all
256 Character values. The local TCTouch adapter now supplies Flush, the defaulted
Order_Meaningful parameter, unordered comparison including duplicate counts,
and defaulted alternatives 3–6 for Validate_One_Of. Omitted empty alternatives
cannot accidentally validate an empty trace; callers must request an empty
first/second alternative explicitly. Flush clears the trace and preserves any
reported failure. Ordered/multiset mismatches, length mismatches, failed asserts,
omitted alternatives and overflow all have executable failure controls.

The adapter follows the selected operations in the pinned
[upstream TCTouch contract](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/support/tctouch.ada).
It remains a local selected-case adapter: full official REPORT/TCTouch support,
annex Implementation_Check grading and general applicability grading are not
claimed. Its trace capacity remains 1,024 rather than upstream's 80.

Local ImpDef adds 0.1-second Switch_To_New_Task/Clear_Ready_Queue advisory delays
for C940005/C940010. They give runnable tasks time to progress on the tested
Darwin arm64 host and do not promise scheduling, priorities or real-time bounds.
Other host validation remains the next roadmap item. No annex applicability
claims or broad target substitutions are inferred from these two constants.

C761004, C940005 and C940010 now execute unchanged as PASS, with full REPORT
completion. They join the regular C-test selection. The other 142 raw baseline
observations and all 149 upstream source hashes are unchanged: **112 C PASS,
six legal-source rejections, one runtime failure and 26 raw B rejections**.

## Reproduce

```sh
python3 -B tests/acats/grading/grade.py --adac build/adac/adac \
    --work build/tests/acats/b-grades --observe
ctest --test-dir build -L b-tests --output-on-failure
ctest --test-dir build -R 'acats.(b_grading_harness|support_protocol)|baseline.acats_b_grades' --output-on-failure
```

Without `--observe` or `--check`, any failed grade makes the command fail.
`--require-pass` explicitly selects that behavior for CTest. `--check` checks
report stability, including known failures; it is baseline evidence, not a
conformance-pass command. Execution logs and per-case JSON reports stay under
the requested work directory. Source hashes are validated before compilation.

Validation: 13 adversarial grader tests, eight real adapter success/failure
controls, 26 actual B compilations with independent legal compilations, and
three unchanged executable support re-probes. The Release suite passes
1,918/1,918 tests on Darwin arm64; all 145 raw baseline cases were re-probed.
