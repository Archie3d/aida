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
retrieved 2026-10-22. CXF2002 and CXF2003 come from `cxf/cxf2002.a`
and `cxf/cxf2003.a`, retrieved 2026-10-22 and stored with `.ada` extensions.
CD3014A and CD3014D come from `cd/cd3014a.ada` and `cd/cd3014d.ada`, retrieved
2026-10-22. CD2A22A comes from `cd/cd2a22a.ada`; its unchanged official
Length_Check helper comes from `support/lencheck.ada`, retrieved 2026-10-22.
C45504A, C45503A and C45611A come from `c4/`, retrieved 2026-10-22.
C35507A comes from `c3/c35507a.ada`, retrieved 2026-10-23.
C36203A comes from [GCC releases/gcc-15 c3/c36203a.ada](https://github.com/gcc-mirror/gcc/blob/releases/gcc-15/gcc/testsuite/ada/acats/tests/c3/c36203a.ada),
retrieved 2026-10-23.
Test files retain their grant of unlimited rights and disclaimer; Length_Check
retains its explicit public-domain authorization. The git blob IDs and SHA-256 digests below pin the exact content;
GCC's release branch itself may change. No download is needed at build/test time.

| Case | Ada 2005 RM | Expectation | Remaining gap / owner |
| --- | --- | --- | --- |
| CD2A22A | 13.3, 13.2 | Compile XFAIL: Pack in the unchanged official Length_Check dependency is unsupported; the case also needs three-bit enum storage | Byte-only Size remains the supported subset; bit packing/Unchecked_Conversion: Phase 8 |
| CD3014A | 13.4, 3.5.1, 3.6, 12.5.1 | PASS: mapped enums retain ordering, array indexing and generic swapping, including character literals | Broader representation checks: Phase 8 |
| CD3014D | 13.4, 3.5.1, 3.6, 12.5.1 | PASS: the same behaviors for enum mappings declared inside a generic package | Broader representation checks: Phase 8 |
| CXF2002 | 3.5.9–3.5.10, 4.5.5, F.2 | PASS: repeated decimal multiplication/division, truncation and Round with matching operand types and both machine radices | Selected Annex F facility; full Annex F remains unclaimed |
| CXF2003 | 3.5.9–3.5.10, 4.5.5, F.2 | PASS: decimal multiplication/division, truncation and Round with different operand types/scales and both machine radices | Ordinary decimal Small and Ada.Decimal.Divide cases remain outside the selection |
| C35A08B | 4.5.5 | PASS: same/mixed fixed operands and explicit Standard multiplication/division | Broader fixed and numeric-context coverage: Phase 8 |
| C354002 | 3.5, 3.5.4 | PASS: modular bounds, Modulus, Min/Max, wrapping Succ/Pred, Image/Value, Pos/Val and Width, including maximum supported moduli | Wide attributes: Phase 7; further modular coverage: Phase 8 |
| C36203A | 3.6.2 | PASS: array Length in universal-integer contexts for one and two dimensions | Library runtime elaboration uses P6b regressions; broader constraints: P6b/P8 |
| C35507A | 3.5, 3.5.2 | PASS: Character, derived and null subtype Width, including image-length consistency | Dynamic scalar bounds use P6b regressions; broader character images/widths: P7/P8 |
| C52005A | 5.2 | PASS: scalar assignment range checks and unchanged target on failure | Broader static arithmetic audit: Phase 5 |
| C53007A | 5.3, 4.5.1 | PASS: nested if and Boolean control flow | Extend selection: Phase 8 |
| C45347A | 4.5.3 | PASS: array/record concatenation in all operand combinations | Broader concatenation selection: Phase 8 |
| C45504A | 4.5.5, 3.5.4 | PASS: Constraint_Error for products outside Integer base range | Further signed arithmetic selection: Phase 8 |
| C45611A | 4.5.6 | PASS: dynamic Integer powers, zero and negative bases, 0**0 | Further signed arithmetic selection: Phase 8 |
| C45503A | 4.5, 4.5.5, 6.4.1 | PASS: predefined Integer REM/MOD, including named Left/Right associations and all operand sign combinations | Broader explicit-operator selection: Phase 8 |
| C45523A | 4.5.5, 3.5.8 | PASS: Machine_Overflows multiplication and zero-division checks | Finite-value floating model: Phase 5; full coverage: Phase 8 |
| C49022A | 4.9, 4.5 | PASS: integer named-number arithmetic and comparisons | Bounded exact evaluation: Phase 5; broader coverage: Phase 8 |
| C59002A | 5.8, 11.2 | XFAIL at compilation: goto and labels unsupported | Phase 6e labels/goto |
| C761002 | 7.6.1, 13.11.2 | XFAIL at compilation: goto and labels in collection lifetime/deallocation test | P6a runtime regressions; P6e transfer cases |

C761002 was retrieved from the same GCC release branch on 2026-10-23.
It exercises collection lifetime, explicit deallocation and exits through nested
scopes, return, goto and exceptions. It is retained unchanged as a compile XFAIL
for the two unsupported goto/label constructs; no upstream executable pass is
claimed. The local TCTouch adapter now also records Touch/Validate traces.

| File | Upstream git blob | SHA-256 |
| --- | --- | --- |
| c45504a.ada | `7cc4af4bcbc77c91afe3c47d83dcdb12ba09af9f` | `33daf0e3e0ef55f4c0443f09f4b4bbe6725f80b1ca59aba9fdbbb36d77d4b97c` |
| c45503a.ada | `0461b0151af5d07aa4f9f62d365da1416307a1ea` | `11726cefb75e74b60dca2eb1e9fc6a9361b91d9ac8990e6d0a43492676bde1d8` |
| c45611a.ada | `3f7a690fb8fdb780e0fa7b59f63dbfefd286671e` | `82b4a12b9a7241ad28979c04490a3c03a3af22d249e3e4093c0f3414348ac444` |
| cd2a22a.ada | `37564d8078763ea4c931ff4eec4efb1b4919e752` | `8da952f626939e85ec9af9fd72d768b6f405abf87f4b0ea489b9961e2ba4cbf3` |
| lencheck.ada | `f8ed1385bff145a2c8f44c6aee4f97d917c8f421` | `6599d2cfdae9dca7602a03d93bbb0a9c5abd14328d4da5ddc09585a13e55f9bd` |
| cd3014a.ada | `ee37df82a196e51a6d4ed7402545e1735d64d8ed` | `7bb4d3f86b4dc71bd24853e92d92abc7fe35a2e2abb78b7cba37db473483fe46` |
| cd3014d.ada | `6ce3f4ce855a4dacb9aeb2c7f7234424d82ad721` | `259ddc783c7433531c1527c4a650a3d63cb3df72b610fc89ce30d49fc778eef6` |
| cxf2002.ada | `984daa97bca695a0dd1b5184849c0fa4233bb245` | `e380eceda0aed84eca205587853f69d69902a2b46dcb25342590fc17edb76183` |
| cxf2003.ada | `133dc48e6c22e9799a3dffa0f78ee2e50e624ee3` | `497d639ae33488b99d04caf6e8887c7838615bee5e823506f2d8e03a87a7fce1` |
| c35a08b.ada | `1750bfa12a21118feff9168d17d05d88c368cab6` | `eabeb71d8bf3a9f9d3324012541e4b119bc6191f531418e05eb62637dbef9b18` |
| c354002.ada | `3129182b70406d33dca826d0f4f505d21ac7f329` | `53ccaca62f66e628b479b53456fa35ca723201093338b93745975b1ecb8d4d90` |
| c52005a.ada | `2c70049c8080ca53f3e5d4fe070915500da5287d` | `927e5fd3db80dfd38fd59619ca65b354893844d64b276a2dfe9c2870626d32bf` |
| c53007a.ada | `bda27b9193b984cd0fd06535e665601e8d910092` | `67662c243b822ba47286d4e52488a6457027afb417a36761f1b56d34f84bfe22` |
| c45347a.ada | `a93ae875ee3a81f23184bfed6be18718a534e7bc` | `757bfceb3d64a4dacd7bc5e0c731ed947769bafcac7db17d3c759f053ed62bc4` |
| c45523a.ada | `ff78eaba756f3d6e2f65475f84281f372a4a6b70` | `60ce8ce3b865cc3651bbef9f79a91586c5ab14aea61015cf6fe9a4380184fe45` |
| c49022a.ada | `d0cfa9d9769d40e1d23ae7f37aae5f71d5791886` | `aaa5fc8afb6ca2d7e77d8d361184f1a727ac53fa78e6914b55ed74c17ca585cb` |
| c59002a.ada | `5210719729379dd00b93c9c304f4aa9abea519e2` | `b8f6fb43975a783abce7384a620acee1278e43d4b75e4d8ff0ad646ea548c9a9` |
| c761002.ada | `5b807bba720ad3cc8b4b0845f33dd704a3e6c00e` | `c71a0098c5bc221924eac3cffed1b4c9fb88cb57fbe8ecd2d6c128ecb9f908d4` |
| c35507a.ada | `0a67765603fe62285e1cadd6d3994392dc3f5b66` | `dbd2ad559f2b315b4ec5fa9eabb853afd2d88292dd2826a78d9cf7f27be42456` |

`support/report.*` is a local adapter implementing only TEST, FAILED, RESULT,
IDENT_INT, IDENT_STR, IDENT_BOOL, EQUAL, COMMENT and NOT_APPLICABLE, sufficient for these cases. It is not the official
REPORT implementation. `support/tctouch.*` supplies local Assert/Assert_Not assertions for C354002 and Touch/Validate traces for C761002. Identity functions return their argument through an
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
CD2A22A is graded with its unchanged official Length_Check dependency from
`upstream/lencheck.ada`; it is not replaced with a weaker attribute-only check.
The pinned diagnostic is the first dependency's unsupported Pack pragma.

Compilation and execution logs remain under `build/tests/acats/work/<case>/`.
Each case has its own build directory and bounded compilation/execution time.
`acats.harness` tests failed checks, missing RESULT, unrelated compilation
failures, changed XFAIL diagnostics, and XPASS grading through real compiler/program invocations.

Alignment selection assessment (2026-10-22): upstream `cd/cd30002.a` directly
covers specified type/object alignments and address divisibility (RM 13.3).
It requires implementation-defined `Impdef` constants, `System.Storage_Elements`,
object alignment clauses and several library units. These are not yet supported
by this subset and single-source C-test harness, so it is deferred rather than
modified or counted as a pass. The local `alignmentclauses` test checks actual
addresses via a C helper in whole/separate modes, including components,
allocators, captured locals and concurrent tasks. CD30001 additionally needs
aliased objects/address conversions; CD30003 tests object Size and packing.
The existing offline ACATS selection remains the regression baseline.

To extend the selection, vendor unchanged upstream sources with provenance and
rights notices, provide their required support operations, add explicit CTest
expectations, and update the [coverage matrix](../../docs/ada2005-coverage.md).
When closing a known gap, remove its rejection expectation and run the executable
case. B-tests, multi-stage/multi-file ACATS construction, implementation-defined
substitutions, broader annex selections and the full official REPORT protocol are future
harness work; do not route them through this C-test runner unchanged.

C36203A retains its upstream contents, including rights and disclaimer.
Its git blob is `f3f7e2bc72f497c64a5132bf5bbeb50c0a3501e4` and SHA-256 is
`52b63badaec465c40c9a4d36bcf9b05db2506a86fde893338f418f0eb7ab2596`.

C48005B comes from [GCC releases/gcc-15 c4/c48005b.ada](https://github.com/gcc-mirror/gcc/blob/releases/gcc-15/gcc/testsuite/ada/acats/tests/c4/c48005b.ada),
retrieved 2026-10-23 and retained unmodified, including its rights/disclaimer.
It passes one- and two-dimensional array allocator bound checks (RM 4.8).
Runtime component layouts, dynamic bound evaluation and ownership use the P6b
regressions; broader allocator/discriminant coverage remains P6c/P8.
Git blob: `c03bde6e04df8b3bcdc9a09cbe7fc9023d5107c0`.
SHA-256: `ec6b9aaf6ee59c074ee179558e2033d866e2988f2a5531421e7eb64f3778ab74`.
