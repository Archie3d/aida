# Selected ACATS B-test grading and support

The Phase 8 harness grades all 26 pinned B-tests independently of the raw
[161-case baseline](acats-baseline.md). It currently records **26 PASS**, comprising
25 with original foundations and B394A01 with an explicitly labelled local
foundation adapter. A nonzero compiler exit alone never establishes a pass. This is a conservative,
local grader for an explicit selection, not the official ACATS grading tool
or a conformance assessment.

## Grading contract

[Policies](../tests/acats/grading/policies.json) name each case's objective,
permitted semantic diagnostic families, legal counterpart and any reviewed
location/recovery exceptions. The unchanged upstream `ERROR:` comments identify
required sites; `OPTIONAL ERROR:` sites may be undiagnosed. Locations are exact
source identities and line numbers. The multiline legacy B36002A declarations
and B33101A fixed-point declaration explicitly allow either declaration line.
The interface ancestry policies below likewise use reviewed declaration spans.
Upstream brace/column annotations are not interpreted; future location extensions require policy review. A diagnostic
in a helper, a different file, or an unmarked line is unexpected, including
errors on `OK` controls.

Every required site needs a matching diagnostic from its allowed group.
Recovery diagnostics need explicit file/line/message permissions and cannot
satisfy a required site. B43002F's secondary empty-association message and reviewed
B37004A component-reference cascades have such permission. Unsupported-feature diagnostics never satisfy a
site, even if a pattern would match them. Missing diagnostics, changed diagnostic
families, parser cascades, unrelated errors, compiler success, build failures,
crashes, timeouts and output-limit failures cannot pass a negative test.

Each grade also compiles a separately authored legal counterpart. The
[shared control](../tests/acats/grading/legal.adb) checks distinct names,
valid derived conversions, discrete/constrained/unconstrained array indices,
record/array aggregates, writable aliased access, positional/named/defaulted
calls, scalar attributes and null procedure defaults. It supplements the legal
controls embedded in the unchanged B-sources. The dedicated
[interface ancestry control](../tests/ada/tasks/interfaceancestry.adb) covers the
selected legal ancestry combinations described below. A legal-control rejection or unexpected output
fails the grade. Tests are frontend-only and do not execute B-programs.

The [reviewed report](../tests/acats/grading/expected.json) retains every missing
site and unexpected diagnostic. Only these complete observed passes are on the
regular `acats;b-tests` CTest labels:

B330001, B33101A, B33102A, B34001B, B34002B, B36001A, B36002A, B37004A,
B394A01 (adapted foundation), B394A02, B394A03, B394A04, B394A05,
B3A0001, B3A0002, B43002D, B43002E, B43002F, B460001, B64002A, B64002C,
B64003A, B650002, B650004, B840001 and BC60001.

B394A01's grade records its `foundation_overrides` mapping and prints
`(local foundation adapter)`. Its original raw observation remains rejected and
ungraded; the adapter is not presented as a corrected upstream pair.

The [non-tasking legality audit](legality-audit.md) repairs and reviews the six
promoted cases, including independent executable controls. Remaining failures
are not waived. Widening a compiler feature requires reviewing its policy and
re-running the full case. Frozen raw B observations remain separate from grades.

## Nested-body returns (Phase 12)

B650004 now grades PASS unchanged: all 22 required return sites receive one
applicable diagnostic, including the ten formerly missed package-body returns.
The innermost body region controls legality independently of the enclosing
callable's storage owner. Package-body returns are rejected in statements,
blocks and handlers; task-body returns outside accept statements are rejected
without unrelated result-type cascades. The existing task-body message is now
an explicitly accepted semantic family, not a recovery allowance.

Its [dedicated legal control](../tests/ada/control_flow/returncontexts.adb)
compiles in the grader and executes through whole-program and per-unit builds,
including direct procedure/function returns, extended returns, accepts and
protected entries. The inline upstream OK controls remain accepted. The other
25 B grades are unchanged; the raw B observation remains rejected and ungraded.

Darwin arm64 validation (11 November 2026): all 2,124 Release CTest checks pass,
including 297 ACATS-labelled checks. Eight focused ASan/UBSan checks and the
sanitizer B650004 grade pass. The full 161-case raw refresh changes only this
case's diagnostics; upstream bytes and hashes remain intact.

## Generic formal subprogram Access (Phase 12)

B3A0002 now grades PASS unchanged at all 22 required sites, with no diagnostics
at its inline legal controls. The generic-body error at line 168 is diagnosed
without an instance: RM [6.3.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-6-3-1.html)
excludes generic formal subprogram profiles from subtype conformance. Such
formals are excluded from access-profile matching during overload resolution.

The other 21 messages already name the applicable rules: 18 designated-profile
mismatches, two protected-convention mismatches and one intrinsic-subprogram
Access restriction. The policy now accepts those reviewed families, with
specific groups at the formal, protected and intrinsic sites. No recovery
allowance or unsupported-feature diagnostic discharges a required site.

The [dedicated executable legal control](../tests/ada/access/callbackprofiles.adb)
checks ordinary and protected callbacks, parameter modes, default parameters,
named/anonymous access parameters and function results. It also exercises an
ordinary wrapper around a generic formal call and a generic formal callback
object. `genericformalaccesserrors` checks uninstantiated generic bodies,
including function formals, anonymous callbacks and a null default. The other
25 B grades are unchanged: 19 PASS / seven FAIL overall.

Darwin arm64 validation (11 November 2026): all 2,129 Release CTest checks pass,
including 298 ACATS-labelled checks. Six focused ASan/UBSan checks and the
sanitizer B3A0002 grade pass. The full 161-case raw refresh changes only this
case's diagnostics; upstream bytes and hashes remain intact.

## Indefinite objects and components (Phase 12)

B330001 now grades PASS unchanged at all 19 required sites. This is a grading
policy repair: the compiler's diagnostic stream and every raw baseline outcome
are unchanged. The applicable semantic families were reviewed per declaration:

| Required marker lines | Declaration category | Required diagnostic family |
| --- | --- | --- |
| 125, 186, 191, 202 | Objects with visible nondefaulted discriminants | Fix the discriminants with a constraint |
| 149, 159, 194, 208 | Objects of unknown-discriminant views | An initializer is required |
| 130, 219 | Array elements with nondefaulted discriminants | A definite discriminated subtype is required |
| 153, 163, 211, 223 | Array elements of unknown-discriminant views | A definite subtype view is required |
| 215 | Protected record component without a constraint | A definite discriminated subtype is required |
| 229, 233 | Class-wide record/array components | A definite subtype is required |
| 240 | Unconstrained array elements, despite an object initializer | A constrained array subtype is required |
| 246 | Unconstrained anonymous array object without an initializer | An initializer or index constraint is required |

Only four multiline array declarations permit an adjacent location:
129–130, 152–153, 162–163 and 222–223. Each span includes the declaration's
opening line and its component subtype line; all other locations remain exact.
Per-site groups prevent an object error from being graded as a component error.
Adversarial tests move errors outside these spans and substitute an unrelated
semantic family; neither mutation can discharge a required marker.

The inline upstream legal declarations remain accepted. The
[dedicated executable control](../tests/ada/types/definiteviews.adb) checks
initialized unknown-discriminant views, defaulted full views and array elements,
constrained arrays/records, class-wide initialized objects and constrained task
and protected objects. It executes in whole-program and per-unit modes with
deterministic IR. The other 25 B grades are unchanged: 20 PASS / six FAIL overall.

Darwin arm64 validation (11 November 2026): all 2,133 Release CTest checks pass,
including 299 ACATS-labelled checks. Three focused ASan/UBSan checks and the
sanitizer B330001 grade pass. All 16 adversarial grader tests pass. The full
161-case raw snapshot and all upstream bytes/hashes remain unchanged; compiler
and runtime code are unchanged.

## Return-form diagnostics (Phase 12)

B650002 now grades PASS unchanged at all 32 required sites. All sites were
already rejected; diagnostics now identify the return's target and applicable
rule rather than reporting a missing result subtype for a non-function body.
Locations remain unchanged, with one diagnostic at each marked site.

| Marker lines | Return rule / accepted diagnostic |
| --- | --- |
| 66, 80, 99, 146, 268 | A procedure cannot return an expression |
| 162, 176 | An entry body cannot return an expression |
| 199, 213 | An accept statement cannot return an expression |
| 68, 72, 82, 101, 105, 148, 152, 270 | An extended return cannot apply to a procedure body |
| 164, 168, 178 | An extended return cannot apply to an entry body |
| 201, 205, 215 | An extended return cannot apply to an accept statement |
| 119, 230, 249 | A simple return from a function requires an expression |
| 124, 235, 254 | An extended return cannot apply to an enclosing extended return |
| 126, 237, 256 | A simple return applying to an extended return cannot have an expression |

Per-site groups distinguish these rules without recovery allowances or widened
locations. A nested function's own extended return remains legal: the target,
not lexical nesting alone, determines which rule applies. Extended returns
rejected for a non-function target retain return-object scope and body analysis,
so independent body errors are still diagnosed; compatible-result-subtype checks
remain specific to functions.

The [legal control](../tests/ada/control_flow/returnforms.adb) executes ordinary,
generic and protected functions/procedures, entry and accept returns, extended
returns with/without handled sequences, handler returns and a nested function
with its own extended return. `returnformerrors` pins exact target-specific
messages and locations, including independent-error recovery. The other 25 B
grades are unchanged: 21 PASS / five FAIL overall.

Darwin arm64 validation (11 November 2026): the full 2,138-check Release run
passed 2,137 checks; its sole failure was an old accept-return message expectation,
subsequently updated. All 14 final affected checks pass. Ten focused ASan/UBSan
checks and the sanitizer B650002 grade pass. The 161-case raw refresh changes
only this case's diagnostic text, preserving every source location and upstream
byte/hash.

## Interface ancestry review (Phase 12)

B394A02–B394A05 now grade PASS without compiler changes. Each diagnostic was
reviewed against its own upstream declaration and the ancestry rules in Ada 2005
RM [3.9.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-9-4.html)
and [12.5.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-12-5-1.html).
Policies permit only the declaration's one-to-three source lines and applicable
semantic families. No recovery permissions or optional-marker overrides were added.

| Case | Required / optional sites | Reviewed reasons |
| --- | --- | --- |
| B394A02 | 34 / 4 | Nonlimited-interface descendants must remain nonlimited; limited extensions require limited parents; limited/synchronized formal derivations require limited ancestors and progenitors |
| B394A03 | 29 / 2 | Task ancestry requires compatible synchronized categories; records cannot implement synchronized/task/protected interfaces; task and protected ancestors cannot be combined |
| B394A04 | 27 / 0 | Protected ancestry requires compatible synchronized categories; records cannot implement synchronized/task/protected interfaces |
| B394A05 | 18 / 0 | Synchronized ancestry requires compatible synchronized categories; records cannot implement synchronized/task/protected interfaces |

B394A02:137 validly receives the specific-interface-progenitor diagnostic:
`GDer01` is a formal derived tagged type (a private extension), rather than an
interface type. This rejects the declaration independently of its nonlimited
ancestry objective. Multiple applicable errors within a declaration are allowed.
The four optional private-completion sites in B394A02 and two in B394A03 retain
the upstream marker semantics: their diagnostics may be absent, and cannot
replace a required error on the original private declaration.

The independent [legal control](../tests/ada/tasks/interfaceancestry.adb)
executes ordinary/limited interface derivations, compatible synchronized/task/
protected descendants, private extensions completed by tasks and protected types,
and generic formal interface/derived contracts with legal actuals. It checks
membership, tags and task/protected operations in whole-program and per-unit
builds, with deterministic IR. This establishes the selected ancestry paths;
broader interface conformance is not inferred.

The 18-test grader suite includes adversarial checks for optional completion
omission/presence, missing original declarations, wrong families and errors moved
outside the reviewed declaration spans. All four grades also pass with the
ASan/UBSan compiler, and the three legal-control checks pass under sanitizers.
The full 26-case grading report now records 25 PASS / one FAIL (B394A01).
All 161 raw baseline observations and upstream source hashes remain unchanged.
The full Darwin arm64 Release run passes 2,144/2,145 checks, including all 304
ACATS checks (25 graded B cases). The sole diagnostics terminal-output comparison
failure passes an immediate isolated rerun; rendering code and expectations are
unchanged. Source/hash validation and `git diff --check` also pass.

## Progenitor types and foundation mismatch (Phase 12)

The official ACAA [B394A01 revision 1.3](http://www.ada-auth.org/cgi-bin/cvsweb.cgi/acats/b3/b394a01.a?rev=1.3&content-type=text/plain)
and [F394A00 revision 1.1](http://www.ada-auth.org/cgi-bin/cvsweb.cgi/acats/support/f394a00.a?rev=1.1&content-type=text/plain)
match the pinned mirror text after CRLF-to-LF conversion. Their official histories
([test](http://www.ada-auth.org/cgi-bin/cvsweb.cgi/acats/b3/b394a01.a),
[foundation](http://www.ada-auth.org/cgi-bin/cvsweb.cgi/acats/support/f394a00.a))
contain no later corrected pair. Both the original 4.1 mirror test and its 4.1W
update reference `Non_Lim_Intf_Priv` and `Lim_Intf_Priv`, absent from the foundation.
Six original diagnostics are consequently name-lookup errors, including four
required sites, and cannot establish the intended interface-progenitor rule.

The separately labelled [local foundation adapter](../tests/acats/grading/foundations/f394a00.a)
preserves the foundation and adds only two private extensions and their full
views: a nonlimited extension of `Non_Lim_Interface` and a limited extension of
`Lim_Interface`. See its [provenance record](../tests/acats/grading/foundations/README.md).
The original vendored test and foundation, source hashes, manifest and raw
observation are unchanged. Only this grade substitutes the local foundation;
the harness rejects attempts to substitute B-test sources or unselected helpers.
The report records the substitution, and helper errors remain unexpected.

With that adapter, all **87 required and ten optional** declarations receive
`a progenitor must name a specific interface type`. Each one-to-three-line span
was reviewed against its declaration. Abstract/tagged records, private extensions,
scalars, arrays, task/protected types and formal private types are not interfaces,
even when they descend from one. The rule follows RM
[3.9.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-9-4.html):
the progenitor subtype must denote an interface. There are no new recovery
allowances; unresolved names cannot satisfy the policy. Optional private-completion
errors cannot discharge required original declarations.

The independent [legal control](../tests/ada/tasks/interfaceancestry.adb) now
also derives from nonlimited and limited private extensions, passes descendants
through generic derived formals and checks their tags and interface membership.
These types are legal ancestors despite being illegal progenitors. The control
runs in whole-program/per-unit modes with deterministic IR. The 20-test grading
suite checks the original name-lookup failure, optional completions, declaration
boundaries, helper errors and restrictions on source substitution.
The current local grading total is 26 PASS (25 original foundations, one adapted).
No compiler or runtime changes were needed. Darwin arm64 validation passes all
2,146 Release checks, including all 305 ACATS checks and 26 graded B cases.
Ten focused ASan/UBSan checks pass, including the full 26-case grading snapshot.
All 161 original raw observations and upstream hashes remain unchanged;
`git diff --check` passes.

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
three unchanged executable support re-probes. The subsequent [legality
audit](legality-audit.md) covers 1,932 Release tests and 21 distinct focused
ASan/UBSan checks on Darwin arm64; all 145 raw baseline cases were re-probed.
