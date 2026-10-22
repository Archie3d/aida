# Selected ACATS baseline

Run after building with `ctest --test-dir build -L acats --output-on-failure`.
The selection is offline and enabled in the normal CTest suite. This is a small
set of inherited Ada requirements applicable to Ada 2005, not a full ACATS run
or a claim of conformity assessment. Ada 2005-specific additions still need tests.

`upstream/` contains unmodified sources from the
[GCC ACATS mirror](https://github.com/gcc-mirror/gcc/tree/releases/gcc-14/gcc/testsuite/ada/acats/tests),
retrieved 2026-10-21 (C49022A, C45523A and C45347A in `c4/`, the other
original cases in `c5/`). C354002 comes from `c3/c354002.a`, retrieved
2026-10-22 and stored as `c354002.ada`. C35A08B comes from `c3/c35a08b.ada`,
retrieved 2026-10-22. Each file retains its grant of
unlimited rights and disclaimer. The git blob IDs and SHA-256 digests below pin the exact content;
GCC's release branch itself may change. No download is needed at build/test time.

| Case | Ada 2005 RM | Expectation | Remaining gap / owner |
| --- | --- | --- | --- |
| C35A08B | 4.5.5 | PASS: same/mixed fixed operands and explicit Standard multiplication/division | Broader fixed and numeric-context coverage: Phase 8 |
| C354002 | 3.5, 3.5.4 | PASS: modular bounds, Modulus, Min/Max, wrapping Succ/Pred, Image/Value, Pos/Val and Width, including maximum supported moduli | Wide attributes: Phase 7; further modular coverage: Phase 8 |
| C52005A | 5.2 | PASS: scalar assignment range checks and unchanged target on failure | Broader static arithmetic audit: Phase 5 |
| C53007A | 5.3, 4.5.1 | PASS: nested if and Boolean control flow | Extend selection: Phase 8 |
| C45347A | 4.5.3 | PASS: array/record concatenation in all operand combinations | Broader concatenation selection: Phase 8 |
| C45523A | 4.5.5, 3.5.8 | PASS: Machine_Overflows multiplication and zero-division checks | Finite-value floating model: Phase 5; full coverage: Phase 8 |
| C49022A | 4.9, 4.5 | PASS: integer named-number arithmetic and comparisons | Bounded exact evaluation: Phase 5; broader coverage: Phase 8 |
| C59002A | 5.8, 11.2 | XFAIL at compilation: goto and labels unsupported | Phase 6e labels/goto |

| File | Upstream git blob | SHA-256 |
| --- | --- | --- |
| c35a08b.ada | `1750bfa12a21118feff9168d17d05d88c368cab6` | `eabeb71d8bf3a9f9d3324012541e4b119bc6191f531418e05eb62637dbef9b18` |
| c354002.ada | `3129182b70406d33dca826d0f4f505d21ac7f329` | `53ccaca62f66e628b479b53456fa35ca723201093338b93745975b1ecb8d4d90` |
| c52005a.ada | `2c70049c8080ca53f3e5d4fe070915500da5287d` | `927e5fd3db80dfd38fd59619ca65b354893844d64b276a2dfe9c2870626d32bf` |
| c53007a.ada | `bda27b9193b984cd0fd06535e665601e8d910092` | `67662c243b822ba47286d4e52488a6457027afb417a36761f1b56d34f84bfe22` |
| c45347a.ada | `a93ae875ee3a81f23184bfed6be18718a534e7bc` | `757bfceb3d64a4dacd7bc5e0c731ed947769bafcac7db17d3c759f053ed62bc4` |
| c45523a.ada | `ff78eaba756f3d6e2f65475f84281f372a4a6b70` | `60ce8ce3b865cc3651bbef9f79a91586c5ab14aea61015cf6fe9a4380184fe45` |
| c49022a.ada | `d0cfa9d9769d40e1d23ae7f37aae5f71d5791886` | `aaa5fc8afb6ca2d7e77d8d361184f1a727ac53fa78e6914b55ed74c17ca585cb` |
| c59002a.ada | `5210719729379dd00b93c9c304f4aa9abea519e2` | `b8f6fb43975a783abce7384a620acee1278e43d4b75e4d8ff0ad646ea548c9a9` |

`support/report.*` is a local adapter implementing only TEST, FAILED, RESULT,
IDENT_INT, IDENT_STR, IDENT_BOOL, EQUAL, COMMENT and NOT_APPLICABLE, sufficient for these cases. It is not the official
REPORT implementation. `support/tctouch.*` supplies the local Assert and Assert_Not adapter for C354002. Identity functions return their argument through an
ordinary separately compiled function; no cross-unit optimization is used.
FAILED records failure and prints its description; RESULT raises Program_Error
on failure. A pass requires normal exit, empty stderr, and exactly the test's
TEST line followed by PASSED. Missing RESULT, unhandled exceptions and failed
checks cannot pass. COMMENT is informational and ignored. NOT_APPLICABLE records a failure: the
selected C45523A case must take its Machine_Overflows=True branch. No case can
be silently skipped as a pass; general applicability/skip grading is future work.

`CMakeLists.txt` explicitly selects executable C-tests. For an expected compile
failure, it records the known diagnostic and the complete normalized diagnostics
in `<case>.diagnostics` (including current parser-recovery cascades). Only a normal
nonzero compiler exit with those exact diagnostics counts as XFAIL. Crashes,
timeouts, unrelated/changed diagnostics, and unexpected compilation success
(XPASS) fail CTest. XFAIL is reported as a passing CTest expectation; verbose
CTest output identifies it as XFAIL. This is not an upstream test pass.
Compilation and execution logs remain under `build/tests/acats/work/<case>/`.
Each case has its own build directory and bounded compilation/execution time.
`acats.harness` tests failed checks, missing RESULT, unrelated compilation
failures, changed XFAIL diagnostics, and XPASS grading through real compiler/program invocations.

To extend the selection, vendor unchanged upstream sources with provenance and
rights notices, provide their required support operations, add explicit CTest
expectations, and update the [coverage matrix](../../docs/ada2005-coverage.md).
When closing a known gap, remove its rejection expectation and run the executable
case. B-tests, multi-stage/multi-file ACATS construction, implementation-defined
substitutions, annex tests and the full official REPORT protocol are future
harness work; do not route them through this C-test runner unchanged.
