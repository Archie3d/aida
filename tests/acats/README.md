# Selected ACATS baseline

Run after building with `ctest --test-dir build -L acats --output-on-failure`.
The selection is offline and enabled in the normal CTest suite. This is a small
set of inherited Ada requirements applicable to Ada 2005, not a full ACATS run
or a claim of conformity assessment. Ada 2005-specific additions still need tests.

A completed-feature [backfill baseline](baseline/README.md) adds 129 pinned,
unchanged cases. Its 80 observed executable passes join the normal ACATS label;
failures, support blockers and ungraded B-probes are recorded separately in
[the triage report](../../docs/acats-baseline.md). Matching a recorded failure
is not an ACATS pass.

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
| C51004A | 5.1, 8.2 | PASS: statement identifiers become visible at the end of block, subprogram, package, generic and task declarative parts | Phase 6e labels/goto complete |
| C59002B | 5.8 | PASS: jumps out of nested if, loop, case and block statements | Phase 6e labels/goto complete |
| C59002A | 5.8, 11.2 | PASS: forward/backward jumps from exception handlers to enclosing sequences | Phase 6e labels/goto complete |
| C85011A | 8.5.3, 8.4 | PASS: package alias chains, use clauses, and current instances of generic/nongeneric packages | P6e package renaming complete |
| C761002 | 7.6.1, 13.11.2, J.1 | PASS: controlled collection lifetime/deallocation across scope exits, goto and exceptions; root compatibility generic renaming | P6a/P6e complete for these scenarios |

C761002 was retrieved from the same GCC release branch on 2026-10-23.
It passes unchanged, exercising collection lifetime, explicit deallocation and
exits through nested scopes, return, goto and exceptions. The root compatibility
`Unchecked_Deallocation` generic renames the existing Ada child unit. The local
TCTouch adapter records and validates its Touch/Validate traces.

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
IDENT_INT, IDENT_STR, IDENT_BOOL, EQUAL, LEGAL_FILE_NAME, COMMENT and NOT_APPLICABLE, sufficient for these cases. It is not the official
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
by this subset, so it is deferred rather than
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

C433001 comes from [GCC releases/gcc-15 c4/c433001.a](https://github.com/gcc-mirror/gcc/blob/releases/gcc-15/gcc/testsuite/ada/acats/tests/c4/c433001.a),
retrieved 2026-10-23 and stored as `c433001.ada`, unmodified including rights
and disclaimer. It passes dynamic applicable index constraints and outside-index
checks for others associations (RM 4.3.3). It has no box associations; those use
local phase 6b regressions rather than an upstream box-conformance claim.
Git blob: `305e010b930f3ed4e7f5ec6d25fb38ebdfc46d6f`.
SHA-256: `dfd1c15d4431ae859efc4e151fc8e2e081edccb76e6a4c98d942862d965a81dd`.

C37206A and C37306A come from
[GCC releases/gcc-15 c3](https://github.com/gcc-mirror/gcc/tree/releases/gcc-15/gcc/testsuite/ada/acats/tests/c3),
retrieved 2026-10-23 and retained unchanged, including rights/disclaimers.
C37206A passes unconstrained subtype aliases and initialized constants of a
record type without discriminant defaults (RM 3.3.1, 3.7). C37306A passes
non-monotonic variant choices and assignment changing a defaulted discriminant
(RM 3.7, 3.8.1, 5.2). Formal constrainedness and concurrent use are covered by
local `defaultdiscriminants*` regressions. C37402A was assessed but deferred:
its unchanged test also requires discriminant-dependent matrix components and
task rendezvous (P6c/P9); it is not counted as a pass or modified.

| File | Upstream git blob | SHA-256 |
| --- | --- | --- |
| c37206a.ada | `d37c794cb9eebe9c2b0ebce9d1fe58d790819ce3` | `c3f6fb0833ca12d0aa2d1bcda84471bdd7dde9cc2e9dd603722a97234558f218` |
| c37306a.ada | `f50fe01958f6b7a7921b010a49bb51c590473d67` | `1547f579ef3b1547f5a7b4a62b46518f455e4d7cd9f4a003425dac9bdc802b3d` |

C37211A and C37108B come from
[GCC releases/gcc-15 c3](https://github.com/gcc-mirror/gcc/tree/releases/gcc-15/gcc/testsuite/ada/acats/tests/c3),
retrieved 2026-10-23 and retained unchanged, including rights/disclaimers.
C37211A passes runtime discriminant constraint/default range failures in
subtypes, array/record components, objects and allocators (RM 3.7.1).
C37108B passes failed per-object array and nested-record constraints, including
private and limited-private views (RM 3.8). Mutable payload ownership and
cross-task collection use are covered by local `discriminant*` regressions.
C37402A still requires task rendezvous (P9); its dependent matrix components
are now supported, but the unchanged case remains deferred rather than counted
as a pass.

| File | Upstream git blob | SHA-256 |
| --- | --- | --- |
| c37211a.ada | `4b718a9ec687889dddc3d30ff0d2dc600dd14d76` | `c9ee082dfe546ebe939a6e829a6d32d19fde39b3f75e191adbf7a99e2e6c915d` |
| c37108b.ada | `9d71e9a729cff416a35454f655ccc4408ba1aeed` | `5bb94ec1285febbf4a61db94429e3c9b3f44e19bf3d67de0539a77f9a333e85b` |

Controlled-component selection (2026-10-24): unchanged
[C760007](https://github.com/gcc-mirror/gcc/blob/releases/gcc-15/gcc/testsuite/ada/acats/tests/c7/c760007.a)
and [C761012](https://github.com/gcc-mirror/gcc/blob/releases/gcc-15/gcc/testsuite/ada/acats/tests/c7/c761012.a)
are stored with `.ada` extensions and retain their rights/disclaimers.
C760007 passes controlled adjustment/copy requirements (RM 7.6).
C761012 checks anonymous variant-object lifetime after failed selection
(RM 7.6.1) and now passes unchanged after Phase 6f tagged discriminants. Local
`controlledvariants` and `controlledvariantlimited` cover controlled variant
components.

| File | Upstream git blob | SHA-256 |
| --- | --- | --- |
| c760007.ada | `c1ddfcb93452dce1c0ceeedabb8b776693219476` | `10a56a3ec80555fdfd5336e6a0c65f90a84491306582ca8489064bd3b6db737f` |
| c761012.ada | `77b9e2253bf7a3f271a3cd8c4381c1763f254887` | `b8e5d2f38a7fe0b6258762c6330934df7f848f3838f6b15aa678051867087c25` |

Record representation selection (2026-10-24): unchanged
[CD4051A](https://github.com/gcc-mirror/gcc/blob/releases/gcc-15/gcc/testsuite/ada/acats/tests/cd/cd4051a.ada)
and [CD1C03H](https://github.com/gcc-mirror/gcc/blob/releases/gcc-15/gcc/testsuite/ada/acats/tests/cd/cd1c03h.ada)
pass record placement and inherited component positions/sizes (RM 13.5.1–13.5.2).
[CD4031A](https://github.com/gcc-mirror/gcc/blob/releases/gcc-15/gcc/testsuite/ada/acats/tests/cd/cd4031a.ada)
checks overlapping variants with bit-packed fields; it is an exact compile
XFAIL for unsupported bit storage, not an upstream executable pass. All three
files were retrieved on 2026-10-24 and retain their original rights/disclaimers.
CD4051B/CD1C04E were assessed and deferred because they change a derived record's
inherited layout; that extension and representation-aware conversions remain
Phase 8. CD1009N additionally requires representation items across a package's
visible/private boundary; the current conservative adjacency rule rejects it.

| File | Upstream git blob | SHA-256 |
| --- | --- | --- |
| cd4051a.ada | `746f82bcd095abc314be330f366f4adf20585d48` | `ce7a16a8cc4ad1adb791fc931ece34101f406dc9c5159bfd106c3756e23e8adb` |
| cd1c03h.ada | `ad84e9196110dfbfdee090794c679ae0b8dfd83d` | `b0ae09307d146d6cec4e3b5b7d0f9fcbb986e5d60c014f68477b0cb5652f4ef6` |
| cd4031a.ada | `936088d6502a611b2a04e5f289743f7668948d0a` | `c32e432ec7e5aafd7c5f541ac1140cc82df7595029c5a62e54571bd264e1db55` |

C3A0013 comes from [GCC releases/gcc-15 c3/c3a0013.a](https://github.com/gcc-mirror/gcc/blob/releases/gcc-15/gcc/testsuite/ada/acats/tests/c3/c3a0013.a),
retrieved 2026-10-24 and stored as `c3a0013.ada` unchanged, including rights
and disclaimer. It covers general access to allocated/aliased objects and
implicitly aliased tagged formals/limited current instances. Private extensions and limited current-instance Access are supported in Phase
6f. Array `Object'Range` index definitions landed with stream-array aliasing
coverage in Phase 7; C3A0013 now executes unchanged as PASS. Local `taggedinstances`
covers self access, inherited defaults, dispatch and allocator accessibility.
Git blob: `b23d4ee11514f0cf96d3a20ca7de30cd0c5efcc6`.
SHA-256: `316ea2029cdabe84823eb0759a82d935e0d38cdd17d072e1328a5eaa18668354`.
C3A0014 tests the superseded Ada 95 aliased-constrained rule and is inapplicable
here. C3A0015 needs storage pools; C3A2002/C460001/C460002 now pass with anonymous
access parameters; C3A2003 still needs range-attribute aggregate choices (P8).
C3A2A01 now passes after formal access/derived types (P6g). C460A01/C460A02
retain later generic gaps and are deferred unchanged; [local evidence and limits](../../docs/general-access.md)
cover the implemented named-access subset.

C460001, C460002 and C3A2002 come from GCC releases/gcc-15, retrieved 2026-10-24,
retained unchanged as `.ada` files with their rights/disclaimers. They execute
as PASS: parameter-to-named-access conversions, forwarding through another
access parameter, and Access of parameter dereferences/renamings, including
allocators and deeper lexical callers. These are inherited Ada requirements
applicable to Ada 2005. They use the existing REPORT adapter without extra support.

| Source | Git blob | SHA-256 |
| --- | --- | --- |
| [c4/c460001.a](https://github.com/gcc-mirror/gcc/blob/releases/gcc-15/gcc/testsuite/ada/acats/tests/c4/c460001.a) | `907b8564f6d3a596b22cef9955e92fa0c81ee1f6` | `d41446b18267f995cea1b19474dded33f884e474a11282597c24ef99a024fc05` |
| [c4/c460002.a](https://github.com/gcc-mirror/gcc/blob/releases/gcc-15/gcc/testsuite/ada/acats/tests/c4/c460002.a) | `945dd567720121eff8e8016748a74e49e2e69773` | `941d65ef9b5ba6098347041e44a13828f87016227cd05af63791c0641ce54ab5` |
| [c3/c3a2002.a](https://github.com/gcc-mirror/gcc/blob/releases/gcc-15/gcc/testsuite/ada/acats/tests/c3/c3a2002.a) | `63ea7008b66b2c996ff20c054560e36ce56fd831` | `49e2e6018c247aecd161390e7951f90b9677032a87c31bd4216ec324c823f624` |

Anonymous array object follow-up (2026-10-24): unchanged
[C36180A](https://github.com/gcc-mirror/gcc/blob/releases/gcc-15/gcc/testsuite/ada/acats/tests/c3/c36180a.ada)
uses inline array object declarations, but also requires `array (A'Range)`
index syntax. That independently tracked P8 gap still rejects the case, so no
new executable ACATS pass is claimed for this parser fix. The exact access-array
example and independent grouped type/bound/initializer semantics are covered by
`anonymousarrayobjects` and rejection regressions.

## Null exclusions (Phase 6d)

Imported unchanged from [simonjwright/ACATS](https://github.com/simonjwright/ACATS/tree/2785c30c1f8577d97ac7b3043882f6818781e9c5),
revision `2785c30c1f8577d97ac7b3043882f6818781e9c5` (the GCC snapshot above predates these Ada 2005 tests).
C3A0030, C3A0020 and C3A0025 execute as PASS. Phase 6g access formal objects
completed the last two scenarios; their old exact rejection snapshots are
retired. C3A0025's limited-record prerequisite landed in 6e. No upstream bodies
are changed. Local `genericaccessobjects`, `genericanonymousobjects`,
`genericaccesscallbacks`, `genericaccesslifetime` and
`genericaccessobjectlibrary_main` add copy/reference identity, strict matching,
defaults, collection cleanup and nested/library replay coverage.

| Source | Git blob | SHA-256 |
| --- | --- | --- |
| [tests/c3/c3a0020.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0020.a) | `b93fb70a57faae00b476a90d8e36ce493af7dfdb` | `40f307512bf27a5d837caa2f7340018b287916405d10f2856361b0996269fc77` |
| [tests/c3/c3a0025.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0025.a) | `ea2cb2911e470ab89815ec59a0ccd53131368e28` | `5fc8dd54d83be8113b469a9c7543e5eafa3d8ee5f7e8a68ca3853e034e92237e` |
| [tests/c3/c3a0030.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0030.a) | `38244a19732935b4ef064337cfa9eff03b4338c7` | `34214aa810bd046823a4707df48bd2171f6f7406bd3b2db568b9ba496bee7ae4` |

## Imported and inherited callbacks (Phase 6d)

The following unchanged sources use the same [ACATS snapshot](https://github.com/simonjwright/ACATS/tree/2785c30c1f8577d97ac7b3043882f6818781e9c5)
as the null-exclusion cases. C3A0006 passes (library callback arrays and calls).
C3A0009 now executes unchanged as PASS after Phase 6f private extensions. Local Ada/C ABI and inheritance regressions
cover the implemented contexts without editing upstream test bodies.

| Source | Git blob | SHA-256 |
| --- | --- | --- |
| [tests/c3/c3a0006.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0006.a) | `effab3465811671874e98662d2b4761aa635078b` | `6462c1aa360510e3674dffd4d8601ac7e52931e0c595f06186ccede9e614adfa` |
| [tests/c3/c3a0009.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a0009.a) | `ba3f2f6e1e7c75435b4fdfb34b5df0dcdca1c203` | `a993a4f8f27bb4675f7fa3557185bb9405cd5bf3c4357fd0ed89a14a893d261b` |

C51004A and C59002B come from [GCC releases/gcc-14 c5](https://github.com/gcc-mirror/gcc/tree/releases/gcc-14/gcc/testsuite/ada/acats/tests/c5),
retrieved 2026-10-24 and retained unchanged, including rights and disclaimer.
They cover statement identifier visibility in declarative parts and transfers
out of compound statements. Their content is pinned below.

| File | Upstream git blob | SHA-256 |
| --- | --- | --- |
| c51004a.ada | `75fa271d0e56b2d3f3f0f8bb2a273380044fc384` | `ede6184296bb0a4254f71792698c6c4d3073b748a8c9b761a1b3883e1fe13b6c` |
| c59002b.ada | `aee5839a7e4eb2776ea6751971269860e6a9f7a3` | `b86a4e7a73760b7e92f83b7fcbe77153b90c58a2ad1a06720aab9798051fa73c` |

C85011A comes from [GCC releases/gcc-14 c8/c85011a.ada](https://github.com/gcc-mirror/gcc/blob/releases/gcc-14/gcc/testsuite/ada/acats/tests/c8/c85011a.ada),
retrieved 2026-10-24 and retained unchanged, including rights and disclaimer.
Its git blob is `538f9c235862c74b9a20afd40e39611922159b95` and SHA-256 is
`a2b84ba57c2f57ec47e2a713604b55698b25cbbceeb18e174a9ddf88f7052f0a`.

## Tagged prefixed calls (Phase 6e)

C413001–C413004 were retrieved on 2026-10-24 from the unchanged
[ACATS snapshot](https://github.com/simonjwright/ACATS/tree/2785c30c1f8577d97ac7b3043882f6818781e9c5),
stored with `.ada` extensions and retaining all rights/disclaimer notices.
All four PASS: value/access prefixes combined with value/access first formals,
inherited and overridden operations, dispatch, ancestor class-wide operations
and named actuals (RM 4.1.3, 6.4). They also exercise nested package bodies
inside library package bodies. Local `prefixedcalls` adds defaults, contextual
overloads, private tagged views, single evaluation, null checks and concurrent
workers; `prefixedcleanup` adds controlled-temporary lifetime checks.

[C413005](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c413005.a)
was assessed and deferred: it needs prefixed-view renaming and generic
subprogram actuals, now explicitly diagnosed and tracked in Phase 8.
It is not counted as a pass or weakened to fit the implemented call subset.

| Source | Git blob | SHA-256 |
| --- | --- | --- |
| [tests/c4/c413001.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c413001.a) | `a1dccaf034623cb538b3d46f42953f908969b790` | `5a354dd650e4dbced9954d2be4f64abf729ebc7045386ea1b4610b562030efe5` |
| [tests/c4/c413002.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c413002.a) | `68b02c01c59740a9737f4e52c18a6d25f1cf229d` | `7ec90991e0504277573829d58f8316c692f042a325fa9901c10d4142c544942e` |
| [tests/c4/c413003.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c413003.a) | `2d2662ac356d1a042ff85719dcac0f1d70467bb3` | `8ef927d6a0ad5e0f98e8118391f0a9a53035aebeefb5029bd017ea034e727b77` |
| [tests/c4/c413004.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c4/c413004.a) | `3d45095afdfc19d8757995b4af6e5e6b4b9012bb` | `7e1fad9c34868f5fa0d5cfed5eea468d41257779ff237e47eb31eb409d2ec08b` |

## Limited and private contexts (Phase 6e)

The following unchanged sources were retrieved on 2026-10-24 from the pinned
[ACATS snapshot](https://github.com/simonjwright/ACATS/tree/2785c30c1f8577d97ac7b3043882f6818781e9c5),
retaining all rights/disclaimers and stored with `.ada` extensions. The harness
now accepts explicit `SOURCES` lists for split cases and an explicit `TEST_NAME`
when the REPORT identity differs from the main source filename. Normal bounded
compilation/execution and exact XFAIL grading still apply.

| Case / inputs | Expectation | Remaining gap / owner |
| --- | --- | --- |
| CA20003: CA200030, CA200031 | PASS: a package reached only by limited with still belongs to the executable partition and elaborates | Further partition/host audit: P8 |
| CA110232: CA110230–CA110232 | PASS: unchanged mutual-reference case, including Unchecked_Access | Broader accessibility audit: P8 |
| CA120012: CA120010–CA120012 | PASS: limited-record full definitions and private contexts | Broader limited/type audit: P6f/P8 |
| CA12002 | PASS: private context visibility inherited by a separate function body | Broader subunit audit: P8 |

All four context cases above now execute as PASS. Local `limitedwith`, `privatewith`
and `taggedincomplete` exercise the implemented views with whole/per-unit/
deterministic compilation, private children, runtime array aliases, four tasks,
allocation and deallocation. `semantics.context_views` covers illegal uses,
source ordering and renamed views; `loading.context_dependencies` covers mixed
cycles, binder membership and incremental dependency invalidation.

| Source | Git blob | SHA-256 |
| --- | --- | --- |
| [tests/ca/ca200030.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/ca/ca200030.a) | `1677a450a6dd38959e05936b870fbe47d33d8257` | `ac75b1af6084d2887e070068275fcd72e06d8b1cb8457681ccf01a23772e1a73` |
| [tests/ca/ca200031.am](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/ca/ca200031.am) | `79d03d26e6a6314316cad1606668e2949ffd75ec` | `5e5198d97e99094733c760057f6e6569da10a0207aebe0101a46b9e98c462660` |
| [tests/ca/ca110230.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/ca/ca110230.a) | `28443767e153960ab514abadda2556ccfaf8e3bc` | `c39cf4bb5e90e5ae7691f5c58a8a517434cf23763cea2406c00f6baf5370cb2b` |
| [tests/ca/ca110231.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/ca/ca110231.a) | `fef10544886ede1c59f00d154b40bc09267882b9` | `751749bcf2edd856a52ca7d16d0bfe46d5df4e7155a51fc4e7c87b2706ef691d` |
| [tests/ca/ca110232.am](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/ca/ca110232.am) | `4d2a92e37cdeede03c9f96e447ce5a97d351c849` | `8ce35c900830911bc0a26c2207368b19545b4cba3ac05fe0a7a227c064fa868c` |
| [tests/ca/ca120010.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/ca/ca120010.a) | `303ccc3e7dde821a0a83f9ea616570a3bbdbe783` | `f6353bd6f5a620afb357993121ec07b53b8405606c79d1076faa3a35c8a22041` |
| [tests/ca/ca120011.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/ca/ca120011.a) | `9b72caa33b113e9d5bf264a46777e0972ff3f86b` | `ca41f91aa6efc55e9038ad71f39e5b18d3b1a38237fef27f80a947acc54942a2` |
| [tests/ca/ca120012.am](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/ca/ca120012.am) | `6787cc51e06b98b97db3cbf30bbe1d0707311ff0` | `737a9b872b8f80f3a25492747cb809c5d81347ade32a3f72c9576cec30ee7f88` |
| [tests/ca/ca12002.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/ca/ca12002.a) | `f87bdd3f3819aeec91d1f9d97f9cf6f7e8ca174d` | `9c7e4281f44e2813506f20db19944039dbb6f96b66964622d3b61c363109af6e` |

## Unchecked object access (RM 13.10)

CA110232 above now passes unchanged. C3A2004 adds contextual overload and
controlling-access coverage from the same pinned Ada 2005 snapshot, unchanged
except for its `.ada` filename. C3A2004 now compiles and executes successfully,
including `Func.all'Access` where `Func` returns access-to-subprogram. CA120012
now passes after limited-record definitions landed with 6e returns.

Local `uncheckedaccess` covers nested and generic scopes, anonymous parameters,
access discriminants/components, overload selection, borrowed array bounds,
mutable records, local tags, dispatch, controlled lifetime and four tasks.
Whole/per-unit execution and deterministic compilation are checked;
`uncheckedaccesserrors` and `uncheckedsharederrors` record legality diagnostics and
`semantics.unchecked_access` excludes object/tag accessibility helpers while
retaining null checks. See [the contract](../../docs/unchecked-access.md).
`callbackdereference` adds closure identity, overload selection, protected/C
callbacks, anonymous forwarding, null checks and four tasks in whole/per-unit
and deterministic modes. `callbackdereferenceerrors` records profile,
convention, privacy and accessibility diagnostics; compiler ASan checks both.
See the [callback contract](../../docs/subprogram-access.md#dereferenced-callbacks).

| Source | Git blob | SHA-256 |
| --- | --- | --- |
| [tests/c3/c3a2004.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c3/c3a2004.a) | `333ededda715908c7d5da59a4f3ab191a68031c0` | `b5f50611a12ff72c129c42effac7c3a4d6c35cfc56717c928c065c2065c3d48c` |

## Separate subunits (Phase 6e)

CA1006A, CA2009A and CA2009D come unchanged from the same
[pinned ACATS snapshot](https://github.com/simonjwright/ACATS/tree/2785c30c1f8577d97ac7b3043882f6818781e9c5),
selected on 2026-10-25 with all rights/disclaimers retained. CA1006A passes
bundled function, procedure and package subunits, including package elaboration.
CA2009D passes generic procedure/function subunits and instantiation.
The already imported CA12002 now passes private-with visibility inherited by a
subunit. Its former stub-rejection expectation has been removed.

CA2009A checks generic package subunits with scalar and constrained-string
actuals. It now executes unchanged as PASS after Phase 6g retained the actual
subtype constraints when matching a formal private type. Local `subunits` covers generic package subunits,
nested subunits, captured frames and four tasks; `semantics.subunits` checks
legality, context isolation and incremental rebuilds.

| Source | Git blob | SHA-256 |
| --- | --- | --- |
| [tests/ca/ca1006a.ada](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/ca/ca1006a.ada) | `7b3527f58e97945978f9913452b7cabc5a92d88d` | `fff62aec5169dc3453548e8b00470d47881bfc9cd004792b475ce38d43d3459d` |
| [tests/ca/ca2009a.ada](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/ca/ca2009a.ada) | `4953045dcf6aa2bb4a8314bfdfa84fb49ee318b9` | `f5b6af9a6d6f5766832f00f975ba4153b9568fc8fbd75deac566dc806ce5df65` |
| [tests/ca/ca2009d.ada](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/ca/ca2009d.ada) | `65b5d811318eefe6ee04e4539727e0a925cae9da` | `9309f5bc3a16c7acc2966ef79a10d792a55a6adceaa118d4c767d61d69f505af` |

## Extended returns and limited construction (Phase 6e)

Unchanged [C650003](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c650003.a)
was selected on 2026-10-25 from the same pinned ACATS snapshot and stored as
`c650003.ada`, retaining all rights/disclaimers. It executes as PASS, checking
limited-record constructors, generic constructors, normal/early extended
returns and abandoning returns by exit, goto and exception (RM 6.5, 7.5).
CA120012 also executes as PASS unchanged. C3A0025 now executes unchanged as
PASS after Phase 6g access formal objects completed its generic prerequisite.

C650A02/C650B01–C650B03 were assessed and excluded: their sources identify
Ada 2012-specific extended-return coverage; they are not Ada 2005 pass evidence.
C650002 needs task rendezvous/select (P9) and is deferred unchanged.
Local `extendedreturns`, `limitedresults` and `returncleanup` add ordinary
scalar/array/tagged/class-wide results, identity-sensitive construction,
allocators, bounds, four tasks and failing initialization/finalization.

| Source | Git blob | SHA-256 |
| --- | --- | --- |
| tests/c6/c650003.a | `2f364c642750d710a352af07bdf9cd9813e57700` | `fbfdf6356f171d3054d1cca4bd355d9b55d34a81d3f1e91358b23ee82950f33e` |


## Generic private and access types (Phase 6g, first increment)

CA2009A is promoted from compile XFAIL to executable PASS. CC54001, CC54002
and CC54003 are copied unchanged from the same pinned
[snapshot](https://github.com/simonjwright/ACATS/tree/2785c30c1f8577d97ac7b3043882f6818781e9c5),
retrieved 2026-10-26, retaining their rights/disclaimers and using `.ada`
filenames. All three execute as PASS with the existing REPORT adapter:

- CC54001: an indefinite private formal and access-to-constant formal implement
  a stack of differently sized strings, allocated and borrowed.
- CC54002: a formal general access-to-variable type implements a list whose
  designated objects are read and updated.
- CC54003: formal object-access and callback types implement a stack of
  functions, each called through its access value.

| Source | Git blob | SHA-256 |
| --- | --- | --- |
| tests/cc/cc54001.a | `eb297d0ecdcd25ee108f60ef2236656f03ceda0b` | `19259fa1a04862198f2fc5d82c53f42e4d7e9b4ec10008b335e81548d6e8658d` |
| tests/cc/cc54002.a | `623f25d6c86fd2c219b8834d9703d18e1f2527d2` | `67b2a028bb92b5d4b41117ce586d7acf6da61877d69837b644ace125c7832a1a` |
| tests/cc/cc54003.a | `d8aaeaf9c815ae822e8419d7d726c3e3ba9bdd3d` | `38c3fd427072ac5bea5d4bd936a7836033dd520a90ccb141bee27c081de4e6f4` |

The later 6g derived/tagged increment now executes C3A2A01 unchanged.
C460A01 still requires class-wide actuals for formal derived types; C460A02
still requires access generic objects. CC3225A requires task rendezvous (P9).
Those deferred cases are not counted as passes. CC54004 now executes after
abstract derived formals (6g).

Local `genericaccess*` covers legal and illegal modifiers, constraints, null
exclusions, callback mode/type/result/count matching, declaration-time contract
errors, nested instances and declaration-site name capture. `genericprivatearrays`
and `genericprivateerrors` cover constrained/runtime/multidimensional actuals
and indefinite-view legality. See the [contract](../../docs/generic-types.md).

## Derived and tagged private formals (Phase 6g)

C393013, C640001 and C760010 now pass their complete REPORT execution protocol,
unchanged, and join the strict baseline ACATS selection. Their original sources
and hashes remain in the [baseline manifest](baseline/manifest.json).
C393013 covers inherited controlling results and tags of null extensions;
C640001 covers callbacks/redispatch through tagged generic profiles; C760010
covers controlled initialization/finalization and exception propagation through
a tagged private formal. C540001 gets past derived-formal parsing but still
rejects expanded character-literal names, tracked in the following 6g work.

C3A2A01, its original F3A2A00 foundation, and CC54004 are copied unchanged from
the same pinned ACATS revision, retrieved 2026-10-26. They retain their original
rights and disclaimers under `.ada` filenames. C3A2A01 executes as PASS, checking
accessibility levels and Program_Error in generic specifications, bodies,
formal tagged-object aliases and access parameters. CC54004 checks abstract
formal-derived types, formal access-to-class-wide types and dispatch through
heterogeneous pointer containers.

| Source | Git blob | SHA-256 |
| --- | --- | --- |
| tests/c3/c3a2a01.a | `8271d4869047b9768e7ac9297009262cd90920b1` | `cd95d2b17fefa1c2fafe660a91579622f0c232fabf48f962c444eb8bff2e4c5f` |
| support/f3a2a00.a | `c839082312e02b799354cdd0e89866b08bf1441a` | `929a2720dec58d22fb460ec647ca4dc978c229bc2bd696542a358c063cb56dd3` |
| tests/cc/cc54004.a | `0023b3a74610d1eaf9c5024914b6a3ea360ad073` | `1e46e506db5fbaf49928d10af1d1fadd102cb01c672de07854d8578da1c5699f` |

Local `genericderived*` adds scalar/array/record/access matching, runtime
record layouts and discriminants, ancestor versus actual override selection,
public views of private overrides, callbacks, nested and separate generic
bodies, four-task runs and exact rejection tests with legal controls.

## Interface-formal contract validation (Phase 6g)

Existing unchanged C394002/C394003 retain their executable PASS status for
limited/nonlimited interface inheritance and shared operations. Local
`genericinterfaceformals`, `genericinterfacecategories`, `genericinterfacenested`
and `genericinterfacelibrary_main` add formal-category, ancestry, reordered
operation-map, callback, diamond and separate-body coverage in whole/per-unit/
deterministic modes with four workers. Their `*errors` controls isolate wrong
actual kinds, missing ancestors, abstract objects, duplicate/non-interface
progenitors, limitedness and additional abstract obligations.

B394A01/B394A02 and their original F394A00 foundation are imported unchanged
from the same pinned revision on 2026-10-26, with original rights/disclaimers.
They target Ada 2005 RM 3.9.4, 12.5.1 and 12.5.5. Their whole-source diagnostics
are pinned as baseline-only B_REJECTED_UNGRADED observations: unsupported
concurrent syntax in the original foundation/test prevents isolating their
expected error groups. Neither rejection is counted as an upstream legality
pass. The local controls above provide feature evidence while P8 B grading and
P9 concurrent interfaces remain open. All original 123 observations are
unchanged; the expanded 125-case cohort has 79 executable passes and 22 ungraded
B observations.

BC55001 was assessed and excluded because it explicitly checks later
AI05/AI12 abstract-result rules. C394001 still needs task-interface implementations,
rendezvous/abort and ImpDef, so remains P9/P8 work. No new executable ACATS pass
is claimed for this interface-formal increment.

| Source | Git blob | SHA-256 |
| --- | --- | --- |
| support/f394a00.a | `81bdfaefb2ddc57f97855ee2c562dd014aa4264f` | `c854401d7b25cc9d4e42b1e77e77a49b3d1fcc9b1ac38130de0753b8267c8237` |
| tests/b3/b394a01.a | `5e693d8f47a2fe15e71d8053aab19a7701cb8bd1` | `5e8c4d2e711517787713ed445aec5897f032ef13367e30ae2f6052d49ac9dc8e` |
| tests/b3/b394a02.a | `4b8f736b968b9aabc33f65dba8b0f27e0c54ceb1` | `adf85f40d9c90ee43a3ca141e46af1d2f4bb81c067730978417a7dcf24627d07` |

## Known formal discriminants and Definite (Phase 6g)

CC1204A and CC3240A are imported unchanged from revision
`2785c30c1f8577d97ac7b3043882f6818781e9c5` on 2026-10-26. Both execute as PASS:
CC1204A checks private/limited-private discriminants that use preceding formal
types, dependent components and generic formal objects; CC3240A checks subtype
membership, conversions, Constrained and selections whose formal discriminant
name collides with an ordinary actual component. Record subtype membership
compares the target subtype's discriminant constraints.

CC51B03 and its FC51B00 foundation are also unchanged. CC51B03 now executes
as PASS, including its class-wide actual for a formal derived type. Its former
compile-XFAIL expectation is removed. Local `genericclasswide*` additionally
checks dispatch wrappers, controlling-result `Program_Error`, nested/library
replay and intrinsic-operation restrictions. Local
`genericdefinite` covers true/false results for scalar, array, discriminated,
defaulted, constrained and class-wide actuals; `genericdiscriminants` checks
Definite and discriminant identities through nested instances. The attribute
remains nonstatic, as checked by `genericdefiniteerrors`.
CC1207B was assessed and deferred because it requires task entries and accept
statements (P9). The existing 125-case baseline observations are unchanged.
All four imported files retain their original rights/disclaimers; upstream
`.a` files are stored with `.ada` extensions.

| Source | Git blob | SHA-256 |
| --- | --- | --- |
| tests/cc/cc1204a.ada | `17e3d7f0f00d0e2a8de240138cb6e07cd32cdec4` | `131fb2f04b857727d5b519ece988d559bd7c7988c41aa08e97a5dacd409a979b` |
| tests/cc/cc3240a.ada | `1983b94294f4aeabe61afb1b536fb4b2a3ba3e53` | `6e724d1a5de26d965e21a974a63e372d4ac6531ccdad114058c34caa55a3c471` |
| tests/cc/cc51b03.a | `0cbeeb46f633d5ce2925af2cd2941f4297dc3079` | `5d1fb7b92f07fe498d072f4bf307e6d9103da244f57711fcaf243b35de5baa72` |
| support/fc51b00.a | `1d2b57e3231a33a05c7bf645694c8689dc27d5f6` | `95ec3d736dc4fc0f7e22657ff0de01ac6ca03589651cb403e2ac59038448ecee` |

## Synchronized formal derived contracts (Phase 6g)

Unchanged B394A03/B394A04/B394A05 are imported from the same pinned revision on
2026-10-26 and retain their rights/disclaimers. They test task, protected and
synchronized ancestry, including generic formal extensions and forbidden
combined task/protected ancestry (Ada 2005 RM 3.9.4 and 12.5.1). Their original
F394A00 foundation is already present. Full-source compilation still rejects
unsupported concrete concurrent/private-extension constructs, so all three are
baseline-only B_REJECTED_UNGRADED probes, not legality passes. No new executable
upstream PASS is claimed for this increment.

Local `genericsynchronized*` isolates valid/invalid ancestors, modifier rules,
synchronized actual matching, abstractness, inherited callbacks, tags,
unknown discriminants and nested/library replay. It executes the supported tag
and callback checks in whole/per-unit/deterministic modes with four workers;
primitive dispatching bodies compile but await Phase 9 concurrent objects for
execution. Existing B394A02 loses two obsolete parser errors; all other 124
prior observations are unchanged. The expanded 128-case baseline has 79 PASS,
16 compile rejections, eight harness blockers and 25 ungraded B observations.
B730008/B730009 were assessed and deferred: their private-extension completions
include concrete task/protected interface implementations (Phase 9). B730011
instead covers private-declaration placement and completion legality, a broader
Phase 8 audit rather than an isolated formal-derived matching scenario.

| Source | Git blob | SHA-256 |
| --- | --- | --- |
| tests/b3/b394a03.a | `0957c26e3793acb229a0204b3c2c53613836e9b6` | `a841defb5c1f45e65f09ec1405fd3169e48f2749c6ee6d82d44dd9ec6b0f1f20` |
| tests/b3/b394a04.a | `9ff3c42389b4001244904a9311d947ee548cefab` | `d717ee36aa03038bf3c7b068c8be0f95400c960fb37f9622aa7670ed699143c3` |
| tests/b3/b394a05.a | `fbb331bb7566d5e1bcb98534ef084e2000f41e84` | `209555a4c8a6e07b174ef8b4ee49bb9b08ac430c94b1408b3b589298bac9c2b7` |

## Abstract formal subprograms (Phase 6g)

CC60001 is imported unchanged from `tests/cc/cc60001.a` in the pinned
`2785c30c1f8577d97ac7b3043882f6818781e9c5` archive on 2026-10-27.
Its full Ada 2005 scenario executes as PASS: abstract procedure/function formals,
box defaults, dispatch to several descendants and an unrelated tagged type,
private extensions, inherited operations and class-wide actual operands.
The local TCTouch adapter's Validate parameter now uses the upstream name
`Message`, supporting the unchanged named associations as well as existing
positional calls. The case joins the focused executable selection; the
128-case backfill cohort and its observations are unchanged.

| Source | Git blob | SHA-256 |
| --- | --- | --- |
| tests/cc/cc60001.a | `08dae248b5b45b0596ce941acb51af94593defa8` | `b85a01d86a7d95175e22fa03bcb410b34b895be7dd91f57d1b00738f72351333` |

Local `genericabstract*` additionally checks controlling access parameters and
results, same-named inherited operations, formal parameter names/defaults,
actual scalar subtypes, explicit/named/box actuals, nested and library replay,
operator mapping and uninstantiated invalid contracts. Four scenarios execute
whole/per-unit/deterministically with four workers; four rejection tests cover
controlling types, matching, defaults, static calls and abstract Access.
Compiler/runtime ASan/UBSan execution covers the four local scenarios in both
modes; rejection tests and CC60001 also pass compiler instrumentation. Generated
Ada memory accesses are not instrumented; LeakSanitizer is unavailable here.
BC60002/BC60003 include Ada 2012-specific objectives/syntax, so they are not
adopted as Ada 2005 legality passes. B-test grading remains Phase 8 work.


## Null procedure defaults (Phase 6g)

Unchanged BC60001 is imported from `tests/bc/bc60001.a` in the pinned
`2785c30c1f8577d97ac7b3043882f6818781e9c5` archive on 2026-10-27. It is a
baseline-only B_REJECTED_UNGRADED probe, not an executable or legality pass.
Its exact observation is four syntax diagnostics for illegal function/abstract
null defaults; parsing errors prevent semantic checking of the remaining
source, including its intrinsic Access rejection. Local `genericnull*` supplies
those separate rejection and executable legal controls. There is no executable
C-test adoption for this increment. All 128 previous observations are unchanged;
the expanded cohort has 79 PASS, 16 compile rejections, eight harness blockers
and 26 ungraded B observations.

| Source | Git blob | SHA-256 |
| --- | --- | --- |
| tests/bc/bc60001.a | `3e53066491ed69675b44429dba72dc8b190b6427` | `88b039164aec66ec5778b818248a32c3a9ee1ee3c8519bb3835d9037bd508396` |

## Attribute subprogram actuals (Phase 6g)

CC1302A and CC3601A are imported unchanged from `tests/cc/cc1302a.ada` and
`tests/cc/cc3601a.ada` in the pinned
`2785c30c1f8577d97ac7b3043882f6818781e9c5` archive on 2026-10-27.
CC1302A executes its complete nested generic default-attribute scenario as PASS,
including Succ/Pred/Image/Value, multiple scalar subtypes and results outside
the prefix subtype but inside its base range. Contract placeholder scalar
bounds are no longer folded into false static overflows.

CC3601A is an exact compile XFAIL at grouped generic formal objects (`V, V1 : T`),
a Phase 8 syntax gap. Its complete operator/attribute scenario is not an
executable pass. Local `genericattribute*` isolates explicit/default/box actuals,
base-subtype profiles, named arguments/defaults, prefix capture, nested/library
replay, binary32/64, fixed/decimal/modular types, Min/Max and intrinsic Access.
These three scenarios run whole/per-unit/deterministically with four workers;
three rejection cases cover profile and attribute-category errors and nonstatic
formal bounds. Compiler/runtime ASan/UBSan passes those scenarios in both modes
and CC1302A in whole-program mode. Generated Ada memory accesses remain
uninstrumented; LeakSanitizer is unavailable. All 129 backfill observations are
unchanged.

| Source | Git blob | SHA-256 |
| --- | --- | --- |
| tests/cc/cc1302a.ada | `c61a310d53e33192320fb45e34550d1069ac1745` | `c15c780bdcff7375dab77ff82e1bda6468f01ac448ff96b95a0068ca4fb375fc` |
| tests/cc/cc3601a.ada | `198f47ecd38997d6f7a19675766993768d736289` | `0130fcec65e597a7ebb18734474dfd263933a91babd65ab824fbf7af7acca579` |

## Formal packages (Phase 6g)

CC70002, CC70003, CC70A02 and CC70B01 execute unchanged as PASS. They cover
complete formal-package contracts, boxed views, formal access-to-subprogram
types, visible operations/copied formals and generic-formal `use` clauses.
FC70A00 and FC70B00 are unchanged shared foundations. Imported 2026-10-28 from
the same pinned archive revision used by the backfill baseline; `.a` files are
stored with `.ada` extensions without changing bytes or rights/disclaimers.
The existing 129-case backfill cohort is unchanged.

CC70001 remains deferred: the unit loader rejects its generic child package of
a generic parent before the formal-package scenario can execute. This is not
an executable pass. Mixed explicit/boxed formal-package lists and `others => <>`
are now covered by local `genericpackagepartial*` and
`genericpartialpackagelibrary_main` scenarios and targeted rejection tests.
No new partial-specific upstream PASS is claimed: CC51009 and CC51011 in the
pinned archive require Ada 2012 formal incomplete types; CC51011 additionally
references a later AI12 correction. Those sources are excluded from the
Ada 2005 selection.

| Upstream source | Git blob | SHA-256 |
| --- | --- | --- |
| [tests/cc/cc70002.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/cc/cc70002.a) | `3e4d9c40b307f296da99b9543bb6b0105e22820e` | `92304981e11f44fe7bc47019c5526ece21a5eecfbb1487120183baf34d933e91` |
| [tests/cc/cc70003.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/cc/cc70003.a) | `d2309fc3695099a14540a84644ed246279f29f6f` | `5b6311497b206b3e073a0e7a48fab7b34830cd4f688636443eb5757671fd4436` |
| [tests/cc/cc70a02.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/cc/cc70a02.a) | `3601ce443e140558e6b7082b888bdff5014ddf9c` | `160a0356e2d92a68b3923531ac47147eb8d772a25b37329832c748e743b573bd` |
| [tests/cc/cc70b01.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/cc/cc70b01.a) | `6c514e17b0651b66184f4319ef0f90c38a44d35b` | `433ebc3a325dfec59dd23e70fd38402c2ad6dbb6c7a294823cecf4b0707c5437` |
| [support/fc70a00.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/support/fc70a00.a) | `e903a13ade37cbeaa96817393134d1f314e3eaa4` | `dbb016afbcedc7a8d30fee2e59ad01facac084b3c9668406ce7195f1d27b84b5` |
| [support/fc70b00.a](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/support/fc70b00.a) | `46b106e0b25a0e3fad98ea1719cad3bdb7337544` | `c68c2cc36258aefa1d967ea40843b03105401798f7971cef4b27d49f2efbbd65` |

## Limited generic `in` objects (Phase 6g)

Existing unchanged C650003 and CA120012 continue to execute as PASS while local
`genericlimitedobjects`, `genericlimitedarrays`, `genericlimitedtagged`,
`genericlimitedcleanup` and `genericlimitedlibrary_main` cover limited formal
object construction directly. They check defaults and formal-subprogram calls,
self-address identity, array shapes/sliding, class-wide dispatch, local/library
storage and controlled cleanup after normal and exceptional completion. The
rejection test checks invalid copied actuals and uninstantiated defaults.
C730004 remains an unchanged baseline PASS: a limited private type's visible
nonlimited completion must allow ordinary aggregate component copying in its
private children. The 129 baseline observations remain unchanged; no new
limited-object-specific upstream PASS is claimed. C650003 and CA120012 also
pass instrumented compiler/C-runtime execution; generated Ada memory accesses
remain uninstrumented and LeakSanitizer is unavailable.

## Generic contract completion (Phase 6g)

The completion increment retains every one of the 129 frozen observations and
all existing unchanged executable selections. CC51B03 and CC70002 additionally
pass instrumented compiler/C-runtime execution. No new contract-specific ACATS
PASS is claimed. Local `genericcontractnested`, `genericcontractreplay`,
`genericcontractregions` and `genericcontractlibrary_main` exercise the nested
class-wide abstract-formal exception, controlling results, source operator
identity, ordinary/private/body overload collapse and library replay.
`genericcontracterrors` rejects visible dispatching homographs, illegal direct
class-wide abstract actuals, invalid uninstantiated templates/defaults and a
private-part null-excluding renaming with a nullable actual. Existing binding,
array/static-matching, accessibility, null and runtime checks remain active.

Phase 6g closes for the supported actual types/profiles. The existing CC70001
generic-child loader deferral belongs to the Phase 8 compiler-completeness
audit; grouped formals, broader legality/staticness and B-test grading also
remain Phase 8. Concurrent implementations and protected-operation actuals
remain Phase 9. Generated QBE memory accesses remain uninstrumented;
LeakSanitizer is unavailable.

## Pragmas and checks (Phase 6h)

Unchanged [C651001](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/tests/c6/c651001.a)
is copied byte-for-byte to `upstream/c651001.ada`, retaining its rights and
disclaimers. It executes as PASS with the complete REPORT protocol, checking
Program_Error when a No_Return procedure completes normally. It also passes
instrumented compiler/C-runtime execution. Generated Ada memory accesses remain
uninstrumented; LeakSanitizer is unavailable.

| Upstream source | Git blob | SHA-256 |
| --- | --- | --- |
| tests/c6/c651001.a | `4d016539556545b22e24c602633c8d260a222ad9` | `f479cadeea490db8630f91c2d7f0d3a9fd6bd9eee91391a867fd9ac718a4c6c3` |

CB42001–CB42003 in the pinned archive use Ada 2012 local Assertion_Policy
pragmas, so they are excluded from the Ada 2005 selection. Local pragma
fixtures cover configuration policies, source-generic policy retention,
lazy evaluation, exception cleanup, check permissions and declaration legality.
No assertion-specific upstream PASS is claimed. All 129 baseline observations
remain unchanged: 80 PASS, 15 compile rejections, eight harness blockers and
26 ungraded B rejections. Categorization, representation and concurrent-policy
pragmas retain their Phase 7/8/9 owners; see the [contract](../../docs/pragmas-and-checks.md).

## Phase 7 unbounded strings

CXA4010, CXA4011, CXA4031 and CXA4032 were retrieved unchanged on 2026-10-28
from the same GCC releases/gcc-14 mirror, under `tests/cxa/` with `.a` suffixes.
They retain their unlimited-rights grants and disclaimers; local `.ada` filenames
let the existing offline harness load them. All four execute as PASS; CXA4032
was promoted from its missing-Maps.Constants compile XFAIL on 2026-10-30.
CXA4030 was retrieved unchanged on 2026-10-30 from the same mirror and directory
and now executes as PASS with mapping-function overloads. These are additional
cases; the 129-case backfill baseline is unchanged. The
[library contract](../../docs/library-collections.md) records the supported API
and remaining restrictions.

| File | Upstream git blob | SHA-256 |
| --- | --- | --- |
| cxa4010.ada | `8646b12b5e48755331ec2b8c009a67bee3d03b0d` | `ed0b5f2fa0d9904dc4aa9f1e5236ecfd5fbfbba3bb829ce4c63da650a58dc1e1` |
| cxa4011.ada | `05388a04ba7767c3ac407b45c80880fd218878e6` | `4a25d9ca6bce4007b1e1d962da6cf69bb977cd83f4c6806242b5a51f170edb1b` |
| cxa4030.ada | `475d00899211454450ccaab5e15dc48f702629e0` | `8006fef5e19b82e357fb9821b4972a5a0da5de10a8836380992f4cd0be14b898` |
| cxa4031.ada | `91bc68ce6e719667cec4767036678271ce018fa0` | `909b2f15c55c4ab683d32da516fd99ba97fd5b335755f4e23e84094ebc33258d` |
| cxa4032.ada | `031d01c6cb79be32a4b418a0842c9a6345241564` | `4e2f2c1659783bee6ac13cdd83b49dd512177762a4e0ac000f09e46185c4a61e` |

## Phase 7 container families

The twelve executable PASS cases CXAI001–CXAI006 and CXAIA01–CXAIA06,
plus the unchanged FXAIA00 foundation, come from the official Ada 2005
[ACATS 3.1DD modification archive](http://www.ada-auth.org/acats-files/3.1/mods/MOD_3_1DD.ZIP),
retrieved 2026-10-28. These are the upstream Amendment versions with Ada
2012-only operations already disabled by their authors; the newer 4.x versions
were not substituted or edited. They exercise all twelve families, including
sorting, generic key views and standard operations. The shared foundation is
passed explicitly to the offline harness. Files retain their unlimited-rights
grants and disclaimers; byte content, including original CRLF line endings, is
preserved. Only local names change from uppercase `.A` to lowercase `.ada`.
The existing backfill observations are unchanged.

Archive SHA-256: `3ec0c44583327bddbeb4d2ba744eaafd468615c61892944cbe57744ef8bf6dff`.

| Archive member | Local file | SHA-256 |
| --- | --- | --- |
| `CXAI001.A` | `cxai001.ada` | `e7b064f5e85f40826795ef521aaf9990a4c7dd3d772e1c738e1895414bd591b8` |
| `CXAI002.A` | `cxai002.ada` | `10acf4e79524e5c36c5616c56855f50f6ea0b6b9d0d3aa623f0310aeb18255ff` |
| `CXAI003.A` | `cxai003.ada` | `aa8273ff7c97d94652537802bcb1b575fd99ad2bf8e8bf6ae8affe902e4bb08d` |
| `CXAI004.A` | `cxai004.ada` | `00bdb9bec7dd9fc73ad8422dfe5e6e643d3b04c40f760c3937fbc1ca0ba93f69` |
| `CXAI005.A` | `cxai005.ada` | `2c1d1e8d2db78ad2b76a17c50dd20e533dc0dbc5539a3839cb37c86d9c49813e` |
| `CXAI006.A` | `cxai006.ada` | `c4db3c0fa84c3db1d84a01e1a2638aca0a72830d6394a91e604e89baef1fef4d` |
| `CXAIA01.A` | `cxaia01.ada` | `7e568b7e7803f85883566560a0fd9dbe08722878fa572a16e3b401ae56e72da7` |
| `CXAIA02.A` | `cxaia02.ada` | `c989e724b82907a42e7ea1bbae7b0eb14563041d3db3a179f3ae271de5744346` |
| `CXAIA03.A` | `cxaia03.ada` | `57663b4b2a7d16cc85837534bd1e8898b1682f022116a7143d6390a304e8466f` |
| `CXAIA04.A` | `cxaia04.ada` | `e8e9c3b9c1495ea2a825fc76e8e6eb95069e3c7d57ab03357388ff321fe15146` |
| `CXAIA05.A` | `cxaia05.ada` | `d81090751200004d4ed163ba3adb10a27f900c7b351de288d506729801d065d8` |
| `CXAIA06.A` | `cxaia06.ada` | `9ba4832171002bb34f9f295210f94a6894ffcebe632b871fae66d9a56ef633ee` |
| `FXAIA00.A` | `fxaia00.ada` | `a039b83ba42305ca63bfcf6792ec57be454dac2fdd423af16e04957cf08060e9` |

## Phase 7 host services and Calendar

CXAH001/CXAH002 (A.17, Ada 2005 APIs) and C96006A/C96007A (9.6, inherited
Calendar requirements) are unchanged executable PASS cases from the same
[simonjwright ACATS snapshot](https://github.com/simonjwright/ACATS/tree/2785c30c1f8577d97ac7b3043882f6818781e9c5),
retrieved from the pinned local archive on 2026-10-29. Their rights notices remain
intact. The local Impdef adapter defines only two host variable names for these
cases; RunAcats sets the readable variable and unsets the writable variable in
the test process. CXAG001 now passes unchanged as an executable C-test, using
Text_IO.Get(File, String) and REPORT.Legal_File_Name. The filename adapter exposes
File_Num = 1 .. 5 and the X/Nam defaults: it uses the conventional X/Y/Z/V/W
prefix and characters 3 .. 7 of the current test name (or explicit Nam).
Test stores the first seven test-name characters, padding short names with '_'.
The local Impdef adapter supplies a POSIX directory name and case-sensitive
filename comparison.
It does not claim the full Impdef protocol. Calendar tests use
the standard J.1 compatibility package. Calendar children remain unimplemented.
The host-service cases are registered on MinGW too; native Windows execution
has not yet been validated. The Impdef names are portable simple ASCII names;
the adapter's case-sensitive comparison suffices for these unchanged scenarios,
which preserve the spelling of the names they construct.

| Upstream path | Git blob | SHA-256 |
| --- | --- | --- |
| `tests/c9/c96006a.ada` | `0f6448bd2f953b97446199507561c670f12366a1` | `bbe069cb5fbb886baf1a337f9c2c8e143207995b0919782c7050d41131c8e0d0` |
| `tests/c9/c96007a.ada` | `15ac5e9b7ed3e4298358f7ee660b4ddd90a39c87` | `8eebe380c976385592d832af937b381fae6508e53dabdd80f7b81bb675fd2b4f` |
| `tests/cxa/cxah001.a` | `dc0f1624f0a671400a00f152bf0c376fd6cc827e` | `922fdbbfbfa966b9b7766e7dedbcd6b0c6a43f2be094fc4a753532a88274cac1` |
| `tests/cxa/cxah002.a` | `c5b6b44968c0fc6ce6e68cf60370430c5b99440e` | `fcd1b3381218a721403593521fec030980e2cb6749999c1163dd8b79cd06d92f` |
| `tests/cxa/cxag001.a` | `f6856d71970fbe12b3c72cd8fd6200a10b110cda` | `3ee9f6f032fdfa78da62e3825e7ae5913fc2a5ecdecdd57690fa1b20a1bce376` |

## Type-aware streams (Phase 7)

Unchanged CDD1001 and CDD2B01/FDD2B00 come from the pinned
[ACATS snapshot](https://github.com/simonjwright/ACATS/tree/2785c30c1f8577d97ac7b3043882f6818781e9c5).
They execute as PASS, checking aliased Stream_Element_Array components and
confirming elementary Stream_Size clauses. REPORT gained the identity Ident_Char
adapter while assessing the related CDD2001 case. CDD2001 currently raises
Constraint_Error at a dependent-component aggregate comparison and is not counted
as a pass; CDD2A01/02 retain conversion-view assignment and derived aggregate/type
gaps. The [stream contract](../../docs/streaming.md) records local evidence.
The Range syntax needed by CDD1001 also promotes unchanged C3A0013 to PASS.

| Upstream source | Git blob | SHA-256 |
| --- | --- | --- |
| tests/cd/cdd1001.a | `3e16f5d4f97db832e1a454fe1dc96db1f771a9d1` | `84bb6c419f5abe0840fadad4978d0391f869da422e767db20f5e9e9b060e5b46` |
| tests/cd/cdd2b01.a | `340d9a7c042979aeb8b43dd699eaf0b1d44e9808` | `af0463bc543ba0d3680b1b929857ee8d5984e49b95d02dd744117ee4908dcbf0` |
| support/fdd2b00.a | `1a1835609e611ee678e0b1cf3230974846f09597` | `d36ddc94faa38c464d2680e94341ba3a28ab93c49a20e9090b00be50e0cd0ac0` |

## Ada.Exceptions (Phase 7)

Unchanged CB41001, CB41003 and CB41004 come from the pinned
[ACATS snapshot](https://github.com/simonjwright/ACATS/tree/2785c30c1f8577d97ac7b3043882f6818781e9c5)
and execute as PASS. They check identity, raising/reraising, saving and null
identity/occurrence contracts. CB41002 was assessed but its task-entry syntax
remains unsupported; it is not counted as a pass. Local wide-name, limited-result,
streaming/partition and four-task coverage is recorded in the
[exception contract](../../docs/exceptions-library.md).

| Upstream source | Git blob | SHA-256 |
| --- | --- | --- |
| tests/cb/cb41001.a | `95ad868feaf3e937847566f16a153e0f593363cd` | `1fb2222e1d4356c9f7e837dcc0cc7a7405e71b515c752d5940c9d00920ef35da` |
| tests/cb/cb41003.a | `aee0b094ce5394e5640ad59e5c86433667e6dd03` | `56ac200b58bc40dca2b46b42bd755601478ea846980c85deb3d8e2d7fd3ca6ef` |
| tests/cb/cb41004.a | `10a300f140f57ddf9b474bc7619fd5b9c4915d41` | `5e34b1822ba8eeb712865ec19c4d9f85d2a3896f94d348d2bf750c14bb22bb60` |

## Phase 7 Text_IO and file lifetime

CE3402A, CE3704A, CE3804A and CE3904A were retrieved unchanged on 2026-10-30
from the GCC `releases/gcc-14` mirror under `gcc/testsuite/ada/acats/tests/ce/`.
They retain their original source bytes, rights grants and disclaimers. They
execute as PASS using the Ada 2005 J.1 `Text_IO` compatibility renaming and the
standard nested I/O generics, without source edits or extra context clauses.
These added cases leave the 129-case backfill baseline separate. They cover
New_Line's input-mode error, Integer_IO default/explicit input, Float_IO signs,
and Enumeration_IO's final token and subsequent EOF.

| File | Upstream git blob | SHA-256 |
| --- | --- | --- |
| ce3402a.ada | `18773f848a287d20aad75624f7545d2cde0b04a2` | `6615c1c9787c0c9b7259557db18070b8ea8b0b1451ec88ee2b8101d8b962c154` |
| ce3704a.ada | `f2325c04ba4a86ba272fd45cc986a4f23b402fb1` | `4f958f715cbf7b6bb7a08d9dea391878616e1e67f483183b78ebe1a5f828fc3f` |
| ce3804a.ada | `c05a1ff1af1d4fdffe57995baeb3646841945f42` | `b1bb8a15cd334849faef6d283e06a9a8e423219a2118eca4ba34e870ef3f070b` |
| ce3904a.ada | `7fe900b6d666746ee15d95a41b790f398190e61e` | `556e27abda0735c49d14e859289de8ee2d8860fd2b27fc18097b123845ed995a` |

[Text I/O contract, validation and remaining limits](../../docs/text-io.md).

## Phase 7 Calendar children

C96008A and C96008B were extracted unchanged on 2026-10-30 from the official
[ACATS 3.1 complete archive](http://www.ada-auth.org/acats-files/3.1/ACATS31.ZIP),
using `C9/C96008A.ADA` and `C9/C96008B.ADA`. Archive SHA-256:
`db2b3a069dda64ac0b5f8f6efd72e762c16f6b9a16f3c7987db562f10b335c1c`.
Original CRLF bytes and rights
grants/disclaimers are preserved. Both execute as PASS: they cover Calendar
composition/decomposition, named profiles/defaults, Clock/System.Tick and
midnight normalization. Existing C96006A/C96007A still execute; the 129-case
backfill baseline remains separate. Child-package coverage comes from the
focused Ada tests and exhaustive C runtime checks in the
[calendar contract](../../docs/calendar.md).

| Archive member | Local file | SHA-256 |
| --- | --- | --- |
| `C9/C96008A.ADA` | `c96008a.ada` | `3ab2401602539a4fc5e693dbde2fc3b583013155c511cdeffa4dd29356c36e17` |
| `C9/C96008B.ADA` | `c96008b.ada` | `68b41ab56335d0f0b53229f5b179438e801ea23753d4ab9cd1554ade01c78dc6` |

## Package categorization (Phase 7)

CA21001 was extracted unchanged from `CA/CA21001.A` in the official
[ACATS 3.1 archive](http://www.ada-auth.org/acats-files/3.1/ACATS31.ZIP)
on 2026-10-30. The archive SHA-256 is
`db2b3a069dda64ac0b5f8f6efd72e762c16f6b9a16f3c7987db562f10b335c1c`;
the stored source SHA-256 is `0a65b3dabb0484153eca15cbdbbdc9b4490817c5d9aad9ab2143b188fc12c271`.
It checks that package subunits inside a preelaborated subprogram may execute
ordinary initialization and statements. This executable C-test uses the local
Report adapter; the pinned 129-case backfill cohort is unchanged. The official
BA21A02/BA21A03 objectives were reviewed when writing local legality controls;
these B-tests are not graded by this harness and no passing claim is made.

## Wide-character conversion foundation (Phase 7)

CXA3004 is stored unchanged from `CXA/CXA3004.A` in the official
[ACATS 3.1 archive](http://www.ada-auth.org/acats-files/3.1/ACATS31.ZIP).
It exercises the J.14 wide conversions through Ada.Characters.Handling.
Retrieved 2026-10-30; archive SHA-256
`db2b3a069dda64ac0b5f8f6efd72e762c16f6b9a16f3c7987db562f10b335c1c`;
source SHA-256 `c6dcac7438acb9471b2f13ad56374e493694b1fca3552f670ed7b8e5b70a35f6`.
The pinned backfill cohort is unchanged. This case does not cover wide text I/O
or the wide string-library families, which remain later substeps.

## Wide text I/O (Phase 7)

CXAB001 is stored unchanged from `CXA/CXAB001.A` in the official
[ACATS 3.1 archive](http://www.ada-auth.org/acats-files/3.1/ACATS31.ZIP).
Extracted 2026-10-31; archive SHA-256
`db2b3a069dda64ac0b5f8f6efd72e762c16f6b9a16f3c7987db562f10b335c1c`;
source SHA-256 `3e1ce7fb49e83eb1cfe62fd66b5e30c69d091be6df2280d1c7c0e026c09a6492`.
It executes as PASS using the local Report adapter, covering wide character and
string external I/O, reset/append/open/close and deletion. Its original rights
and disclaimer are preserved; the pinned backfill cohort is unchanged.

## Wide string families (Phase 7)

CXA4012–CXA4023, CXA4025, CXA4029, CXA4033 and CXA4034 are stored unchanged
from `CXA/<name>.A` in the official
[ACATS 3.1 archive](http://www.ada-auth.org/acats-files/3.1/ACATS31.ZIP).
Extracted 2026-10-31; archive SHA-256
`db2b3a069dda64ac0b5f8f6efd72e762c16f6b9a16f3c7987db562f10b335c1c`.
All sixteen execute as PASS, covering wide maps/constants, fixed/bounded/
unbounded operations, mapping callbacks, truncation and slice errors. The local
REPORT adapter adds the identity function Ident_Wide_Char for CXA4034. Original
rights/disclaimers and the pinned baseline cohort are unchanged.

| Source | SHA-256 |
| --- | --- |
| CXA4012 | `62a0bde94ae2269c6dfc89f633dfef9ff3923a60a028783ea9bc0519e1bf3c2a` |
| CXA4013 | `9b083855d92d8561b7da570011a4d6256ec1658251b2624b653a9f54ceadbe8b` |
| CXA4014 | `40b439836b51ba8ca9b0b45afbb67a529998307d38c0889031a7045d9dcbc7ab` |
| CXA4015 | `1b08aee0bca50fdbd70293ab96f9fcfd95c5da91f539586e998cd520a388257a` |
| CXA4016 | `27ff344708653173d8d287a61fb860d19040d5ea4b9aff1acea04d7ef86bde35` |
| CXA4017 | `cb967745bfe94a32af1b17f83973f6122697b60bb3c978bc0a0706a198758798` |
| CXA4018 | `c75e70ab46bfb2212c8ddaa24647d4c55760d80f710025fca4ee5b407939c0c5` |
| CXA4019 | `0cfabe2853873ce42c543a58b8cbead13dc6916445fd6b609fdfe40906915b08` |
| CXA4020 | `4bc9af58e2dcc9e8b575a504eeaed48bd3b4a9d00d7c52478ec0a6d1573155ea` |
| CXA4021 | `d78329fd759216a9718b0c5f1e64ccd2155f321d37eea63b97580f2e65362324` |
| CXA4022 | `81acb82e5501eb3d7c331ff1dd37314c31a0f13929925122875574ad18c88952` |
| CXA4023 | `9d2ec0168d4194a4a106a6754386e58f14e33998f24030f038dc4ea41b44dfac` |
| CXA4025 | `5c447a3ceb72691d97e8a75e87b1dbff89e1e2b08e96d7ff48dff07c65d129a8` |
| CXA4029 | `ab2cafc19a2bf07213eb8d5e46b31926cfab0fd325ad8bc47dd0c606a2238ccb` |
| CXA4033 | `627cf15de401e977e31b49c606a634bfa9f459db572b4d772792b0b032f20a5c` |
| CXA4034 | `fdad471a74e85593d562a9a121c0df5e2290954e2a07fc6f2470e06bf78f07f8` |
