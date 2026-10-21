# Selected ACATS baseline

Run after building with `ctest --test-dir build -L acats --output-on-failure`.
The selection is offline and enabled in the normal CTest suite. This is a small
set of inherited Ada requirements applicable to Ada 2005, not a full ACATS run
or a claim of conformity assessment. Ada 2005-specific additions still need tests.

`upstream/` contains unmodified sources from the
[GCC ACATS mirror](https://github.com/gcc-mirror/gcc/tree/releases/gcc-14/gcc/testsuite/ada/acats/tests/c5),
retrieved 2026-10-21. Each file retains its grant of unlimited rights and
disclaimer. The git blob IDs and SHA-256 digests below pin the exact content;
GCC's release branch itself may change. No download is needed at build/test time.

| Case | Ada 2005 RM | Expectation | Remaining gap / owner |
| --- | --- | --- | --- |
| C52005A | 5.2 | PASS: scalar assignment range checks and unchanged target on failure | Broader static arithmetic audit: Phase 5 |
| C53007A | 5.3, 4.5.1 | PASS: nested if and Boolean control flow | Extend selection: Phase 8 |
| C59002A | 5.8, 11.2 | XFAIL at compilation: goto and labels unsupported | Phase 6e labels/goto |

| File | Upstream git blob | SHA-256 |
| --- | --- | --- |
| c52005a.ada | `2c70049c8080ca53f3e5d4fe070915500da5287d` | `927e5fd3db80dfd38fd59619ca65b354893844d64b276a2dfe9c2870626d32bf` |
| c53007a.ada | `bda27b9193b984cd0fd06535e665601e8d910092` | `67662c243b822ba47286d4e52488a6457027afb417a36761f1b56d34f84bfe22` |
| c59002a.ada | `5210719729379dd00b93c9c304f4aa9abea519e2` | `b8f6fb43975a783abce7384a620acee1278e43d4b75e4d8ff0ad646ea548c9a9` |

`support/report.*` is a local adapter implementing only TEST, FAILED, RESULT,
IDENT_INT and IDENT_BOOL, sufficient for these cases. It is not the official
REPORT implementation. Identity functions return their argument through an
ordinary separately compiled function; no cross-unit optimization is used.
FAILED records failure and prints its description; RESULT raises Program_Error
on failure. A pass requires normal exit, empty stderr, and exactly the test's
TEST line followed by PASSED. Missing RESULT, unhandled exceptions and failed
checks cannot pass. This adapter has no NOT_APPLICABLE handling: extend the
protocol explicitly before selecting cases that require it.

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
