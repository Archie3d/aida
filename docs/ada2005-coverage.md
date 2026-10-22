# Ada 2005 coverage baseline

This is a gap-review skeleton, not a conformance claim or an exhaustive list of
Ada requirements. RM references use the [Ada 2005 RM](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-TTL.html).
Each row records implemented behavior, remaining restrictions/checks, evidence,
and the roadmap owner. A completed roadmap audit covers its documented subset.

Classification: **Core** = Ada 2005 core (including inherited Ada 95 rules);
**Choice** = implementation-defined behavior requiring documentation;
**Extension** = behavior outside the language target; **Annex** = selected
Specialized Needs Annex facilities, without annex/profile compliance claims.
`—` in ACATS means no selected upstream test yet, even if regressions pass.
Regression names are CTest names or source-name prefixes under `tests/ada/`;
expand prefixes with `ctest --test-dir build -N`. Phase 8 must refine broad
rows into individual legality/dynamic requirements and add missing units.

## Completed Phases 1–4

| RM / class | Behavior audited | Restrictions / missing checks | Regression evidence | ACATS | Roadmap owner |
| --- | --- | --- | --- | --- | --- |
| 2.9 / Core | Reserved words recognized | Unsupported syntax diagnosed, not implemented | `reserved*` | — | P1 Reserved words; P6/P9 |
| 2.8, 10.2.1 / Core | Import; advisory Inline/Pure/Preelaborate | Other pragmas rejected; categorization not enforced | `pragma*`, `pragmas` | — | P1 Pragmas; P6h/P7 |
| 13.1, 13.3 / Core, Choice | Validated Size, Small, decimal Machine_Radix and adjacent first-subtype Alignment clauses; subtype/object Alignment queries and record padding | Alignment limited to 0/1/2/4/8, no weakening or scalar/array stride padding; object clauses and tagged/task/protected types rejected; record clauses pending; Size remains byte-addressable and static, with limited/controlled layout changes rejected | `size*`, `representationerrors`, `alignmentclauses`, `alignmenterrors` (whole/separate/determinism, concurrent tasks) | CD30002 assessed; blocked by Impdef, Storage_Elements, object clauses and multi-unit harness | P1 Representation clauses; P5/P6c |
| 13.3 / Core, Choice, Extension | Size controls static scalar storage and composite trailing storage, without changing base arithmetic; sized actual/result adaptation and prefix-format streams | Named-subtype Size is an AIDA extension (RM permits first subtypes); byte multiples only, no bit packing, dynamic or nonconfirming tagged/controlled/limited/task/protected sizes | `sizestorage`, `sizestreams`, `sizetasking`, `sizestorageerrors`, `semantics.type_storage`, whole/separate/determinism and ASan/UBSan | CD2A22A compile XFAIL for official Length_Check Pack dependency; no passing claim | P5 Representation clauses; P6c record clauses; P8 freezing/bit storage |
| 13.4 / Core, Choice | Static enumeration codes, inherited/overridden mappings, 8/16/32-bit storage; positions retained for attributes, loops, indexing and Ada scalar calls; C scalar code marshalling and checked scalar streams | Adjacent first-subtype clauses only, signed 32-bit codes; Boolean/Character-derived mappings and clauses after private-view subtype declarations rejected; scalar values use positions in Ada call ABI | `enumrepresentation`, `enumstreams`, `enumtasking`, `enumrepresentationerrors`, `runtime.enumeration` (whole/separate/determinism, concurrent tasks) | CD3014A, CD3014D | P5 Representation clauses; P8 broader code widths/freezing |
| B.1 / Annex | C/Ada imports, three positional arguments, nonempty external name | Other conventions and imported controlled-result ownership unsupported | `importconvention*` | — | P1 Imports; P7 ABI |
| 10.1.3 / Core | Body stubs diagnosed with recovery | Separate subunits not implemented | `subprogramstuberrors` | — | P1 Stubs; P6e |
| 12.5 / Core | Supported formal categories parsed strictly | Derived/access/interface formals and trailing syntax rejected | `genericformal*`, `deallocationformalerrors` | — | P1 Generic formal types; P6g |
| 3.9.3, 5.8–5.9, 9.5–9.8, 10.1.2 / Core | Named diagnostics for unsupported constructs | Abstract operations, goto/labels, task entries/select, limited/private with missing | `unsupported*` | C59002A XFAIL (goto) | P1 Unsupported syntax; P6e/P6f/P9 |
| 11, 7.6.1 / Core | Exception, trace, allocation and finalization state per task | Collections restricted to owning context | `runtime.task_context`, `runtime.array_storage` | — | P2 Per-task context; P6a |
| 11 / Core | Generated context access via traceback entry/exit | No hidden TLS operand; cross-host validation still open | `runtime.generated_contexts`, `determinism.generatedcontexts` | — | P2 Generated access; P8 hosts |
| 3.5, 13.13.2 / Core, Choice | Caller-owned image/fixed/stream result buffers | Lengths bounded by signed 32 bits; streams mostly representation copies | `imageresults`, `streamresults`, `runtime.result_buffers` | — | P2 Result buffers; P5/P7 |
| A.10, 3.9, A.15 / Core | Shared I/O/tag tables and atomic exit status | Host I/O behavior; full task/collection sharing still open | `runtime.shared_tables`, `runtime.blocking_io`, `filesnapshots` | — | P2 Shared run-time tables; P6a/P8 |
| 9.10 / Core, Choice | POSIX/winpthreads threading and monotonic waits | Native lock-free 8–64-bit atomics required; MSYS2 validation open | `runtime.threading` | — | P2 Threading layer; P8 hosts |
| 7.6.1, 9.3 / Core | Masters await, finalize and release inside-out | Full rendezvous/abort semantics missing | `runtime.master*`, `determinism.masterhooks` | — | P2 Masters; P6a/P9 |
| 10.1.1–10.1.4 / Core | Library identity, spec/body loading and cycle checks | Generic renaming completions unsupported | `loading.*`, `ada.load_main`, `ada.child_main`, `ada.bundle_main`, `ada.completion_main` | — | P3 Unit identity; P6e |
| 10.1 / Extension | Source bundles and custom filenames | Not standard Ada source organization | `loading.*` | — | P3 Unit identity |
| 10.2 / Core | Binder ordering, body elaboration flags, deferred generic expansion | Serialized semantic interfaces absent | `binding.elaboration_order`, `elaboration*`, `forward*` | — | P3 Elaboration order; P8 |
| 10.2.1 / Core | Elaborate/Elaborate_All/Elaborate_Body placement and dependency checks | Other categorization checks pending | `elaboration.pragmas`, `pragmaelaborationerrors`, `ada.ep_main` | — | P3 Elaboration pragmas; P7 |
| 6.3.1, 7.1–7.3, 8.3–8.4 / Core | Homographs, visibility, full spec/body conformance and required bodies | Package bodies optional without Elaborate_Body; broader gap review pending | `duplicate*errors`, `profile*`, `visibility*`, `requiredcompletionerrors`, `semantics.visibility_conformance` | — | P3 Visibility and conformance; P8 |
| 5.6, 7.2 / Core | Block locals/captures/cleanup during package elaboration | Library-level runtime bounds missing | `elaborationblock*` | — | P3 Blocks during elaboration; P6b |
| 4.3 / Core | Array shape/sliding/choices and ordered record aggregate temporaries | Box defaults and general limited aggregates missing | `aggregateaudit*`, `aggregatecleanup`, `aggregateboxerrors` | — | P3 Aggregates; P6b/P6e |
| 4.6, 6.4.1 / Core | Checked per-dimension array conversion and normal-return copy-back | Cross-type views require identical component subtypes; limited controlled conversions missing | `arrayconversion*`, `controlledconversions` | — | P3 Array conversions; P6 |
| 3.4, 4.6 / Core | Inherited constraints/profiles and derived scalar categories | Writable non-array conversions, new discriminants, early private derivation unsupported; characters one byte | `derived*` | — | P3 Derived types; P6d/P6f/P7 |
| 3.5, 3.6, 4.1.4 / Core | Pos/Val/Succ/Pred/First/Last/Range checking and single evaluation | 64-bit array bounds now covered by P5 descriptor row | `attributeaudit*`, `attributebounds` | — | P3 Attributes; P5 |
| 3.9.2, 6.1, 13.14 / Core | Primitive overriding, defaults/results and object/body freezing | General freezing, tagged discriminants, private extensions, anonymous controlling access missing | `primitiveaudit*`, `primitiveplacementerrors` | — | P3 Primitive operations; P6d/P6f |
| 3.7–3.8, 4.5.2 / Core | Nested variant layout, active equality, assignment checks | Static explicit discriminants only; mutable/runtime discriminants and controlled variant components missing | `nestedvariants*` | — | P3 Variant records; P6c |
| 6.4.1, 8.5, 11, 12 / Core | Cross-feature defaults/overloads/renames/ownership/cleanup | Regression combinations only, not exhaustive legality coverage | `interaction*`, `semantics.interaction_modes` | — | P3 Feature interactions; P8 |
| 3.11, 7.5, 7.6.1, 9.1–9.3 / Core | Entry-free tasks, grouped activation, masters, task components/allocators | Noncopying enforced; entries, discriminants, tagged/variant composites, aggregates/results missing | `tasks`, `taskallocations`, `tasklibrarymain`, `taskactivationfailure`, `task*errors`, `runtime.tasking*` | — | P4 Tasks; P9 |
| 9.4, 9.5.1 / Core | Noncopyable protected objects/operations, lifecycle lock and hidden receiver | Discriminants/interfaces/operation renaming/access/generic actuals missing; external self-calls and task creation in actions raise Program_Error | `protected*`, `runtime.protected_lifecycle` | — | P4 Protected objects; P9 |
| 9.5.2–9.5.3 / Core | Single protected entries, barriers, FIFO, Count and deferred copy-back | Barrier exceptions become Program_Error; families/select/requeue missing | `protectedentries`, `entry*`, `runtime.entry_lifecycle` | — | P4 Protected entries; P9 |
| 9.8, 7.6.1 / Core | Task-owned abort-deferral depth; balanced protected actions/entry servicing and chain/view finalization; explicit emitter calls/returns and shared master exits | [Contract](abort-deferral.md); abort requests/delivery/cancellation, default Initialize, whole controlled assignments and master-wait deferral await P9 | `runtime.abort_deferral`, `abortdeferral` whole/separate/determinism, protected/entry lifecycle | No executable abort selection until P9; unchanged selected ACATS is not abort coverage | P6a foundation complete; P9 delivery |
| 9.6 / Core; D.8 / Annex; 3.5.9 / Choice | Duration, relative/absolute delay and monotonic Real_Time | Duration Small = 2**(-30), 64-bit ticks; Real_Time nanoseconds; no hard real-time guarantees; protected delay raises Program_Error | `time*`, `runtime.time_lifecycle` | — | P4 Time; P7 Calendar/P9 annex |
| C.6 / Annex, Choice | Sequentially consistent Atomic/Volatile scalar accesses and snapshots | Aligned RAM only; shared composite parts/component pragmas unsupported | `shared*`, `semantics.shared_ir` | — | P4 Shared variables; P8 |
| C.7.1, 9.9 / Annex, Core | Task identity/status and entry Caller | Protected entries execute in caller task; Abort_Task missing | `taskidentity*` | — | P4 Task identity; P9 |

## Additional baseline probes

| RM / class | Behavior | Restrictions / missing checks | Regressions | ACATS | Owner |
| --- | --- | --- | --- | --- | --- |
| 5.2 / Core | Scalar assignment checks preserve the old value on failure | Assignment subtype checks remain dynamic; bounded static arithmetic is recorded below | `scalarcopychecks` | C52005A PASS | P5 Static arithmetic; P8 |
| 4.9, 4.5 / Core, Choice | Exact bounded scalar evaluation, universal integer named numbers, final binary rounding and static-check diagnostics | [127-bit integer/rational and 64-bit scaled fixed limits](static-arithmetic.md); full staticness/string/formal-type audit remains open | `staticarithmetic`, `staticarithmeticerrors`, `determinism.staticarithmetic`, `integerliteral*` | C49022A PASS | P5 Static arithmetic complete for the documented subset; P8 gap review |
| 3.6, 4.1.1–4.1.2, 4.6 / Core, Choice | 64-bit array bounds/lengths/byte strides, shared view ABI, checked sizes/offsets, sliding and ownership | [Descriptor ABI v2](array-descriptors.md); signed 64-bit capacity, contiguous views, 32-bit text API bridges; public large I/O work remains | `arraydescriptor*`, `runtime.array_descriptors`, result buffers, whole/separate/determinism and sanitizer checks | — | P5 Array descriptors; P7 I/O/P8 validation |
| 4.5, 6.4.1, 6.6 / Core | Explicit unqualified operators accept positional, named and mixed associations; predefined Left/Right (unary Right), user-defined names, reordering and result contexts participate in resolution | Existing expanded-name restrictions are unchanged; wider operator visibility/profile coverage remains for Phase 8 | `namedoperators`, `namedoperatorerrors`, whole/separate/determinism; generic formal bindings, static folding, operand evaluation counts and exception checks | C45503A PASS | P5 pre-Phase-6 follow-up complete; P8 coverage extension |
| 3.5.4, 4.5.3–4.5.6 / Core | Inline checked signed 32/64-bit arithmetic, including negation, abs, rem/mod and powers; ordinary exception propagation and protected cleanup | [Checked arithmetic contract](checked-arithmetic.md); general 64-bit multiplication needs an unsigned bound division | `checkedarithmetic` (88,000 oracle comparisons per mode), `checkedtasking`, `overflow32/64`, mode/determinism and sanitizer checks; IR helper-call exclusion | C45504A/C45611A/C45503A PASS | P5 Checked arithmetic speed complete; P8 operator-profile coverage |
| 4.5.3 / Core | Nonlimited rank-one concatenation, all four profiles, RM bounds, wide/composite/private/controlled components | [Concatenation contract](array-descriptors.md#concatenation); signed 64-bit storage capacity; wide character types remain Phase 7 | `concatenation`, `concattasking`, `concaterrors`, `runtime.concatenation`, mode/determinism and sanitizer checks | C45347A PASS | P5 Concatenation; P7 wide strings/P8 validation |
| 3.5 / Core, Choice | Floating Value, base results, decimal/based syntax, exact single rounding, malformed/overflow checks | [Floating Value contract](floating-point.md#floating-value); Float_IO token audit remains Phase 7; String length still Integer | `realvalue*`, `runtime.real_value`, whole/separate/determinism and sanitizer checks | — | P5 Floating Value; P7 I/O |
| 3.5.7–3.5.8, 4.5.5–4.5.6 / Core, Choice | Checked binary32/64 arithmetic, negative integer powers, Machine_Overflows/Machine_Rounds True, zero-divisor and conversion checks | [Default floating environment and finite-value model](floating-point.md); static capacity limits, full model attributes and Annex G accuracy remain restricted/missing | `realedges`, `realtasking`, `realedgeerrors`, `runtime.real_arithmetic`, mode/determinism checks | C45523A PASS | P5 Real edge cases; P8 gap review |
| 5.3, 4.5.1 / Core | Nested conditionals and Boolean short circuit | Selected control-flow paths only | `control` | C53007A PASS | P8 coverage |
| 3.5, 3.5.4, 4.5, 4.6, 5.4 / Core, Choice | Unsigned modular storage and all moduli 1..2**64; arithmetic/logic, comparisons, conversions, case choices, scalar attributes and Ada 2005 Mod | Signed 64-bit array-index subtypes and variant choices; no compact unsigned 32-bit storage; wide attributes pending | `modular*`, `attributeaudit`, `runtime.modular`, mode/determinism and sanitizer checks | C354002 | P5 Modular types; P7 wide attributes |
| 12.5.2 / Core | Modular formal types, matching and signed/modular distinction | Broader generic staticness/contract audit remains | `modularwide`, `modularwideerrors`, `genericformalerrors` | — | P5 Modular types; P6g/P8 |
| A.10.8 / Core | Modular_IO file/default/string Get and Put; decimal/based syntax, widths, delimiters, growing file input and Data_Error/Layout_Error | General Text_IO line/page accounting follows existing implementation; full library audit pending | `modulario`, mode/determinism and sanitizer checks | — | P5 Modular types; P7 I/O |
| 3.5.9–3.5.10, A.5.4 / Core, Choice | Signed 64-bit counts, wider binary scales, explicit Small, dynamic local bounds, scalar/formatting/machine attributes and exact text conversion | [Binary Small range and adjacent clauses](fixed-point.md); compact storage, legacy delta constraints and general freezing pending | `fixedwide*`, `fixedtext*`, `fixedtasking`, `runtime.fixed_arithmetic`, mode/determinism and sanitizer checks | — | P5 Ordinary fixed point; P6b/P8 |
| 4.5.5 / Core | Mixed fixed operands with numeric result contexts, Integer scaling, Standard operations and Ada 2005 primitive restrictions | Existing bounded static evaluation; decimal results truncate and Round rounds once | `fixedoperators`, `fixedoperatorerrors`, `fixedwide`, `runtime.fixed_arithmetic` | C35A08B PASS | P5 Ordinary fixed point; P8 |
| 3.5.9–3.5.10, 4.5.5 / Core, Choice | Decimal types with 1–18 digits, exact decimal scaling, digits constraints, truncating arithmetic, Round, mixed conversions and decimal formals | [Scale/storage limits and local elaboration checks](decimal-fixed-point.md); legacy delta constraints, extra model attributes and full Annex F remain open | `decimalpoint`, `decimalchecks`, `decimalerrors`, `decimaltasking`, `runtime.decimal`, mode/determinism and sanitizer checks | CXF2002/CXF2003 PASS (selected Annex F cases) | P5 Decimal fixed point; P6b/P8 |
| 13.13.2 / Core, Choice | Checked eight-byte fixed counts; Input subtype checks and Read copy-back checks, including dynamic bounds | Native-order representation; general type-aware/user-defined streaming pending | `fixedstreams`, `decimalstreams`, whole/separate/determinism and sanitizer checks | — | P5 Ordinary fixed point; P7 streaming |


## Predefined-unit inventory

All rows are **Core** unless marked Annex. These are source-present units, not
claims that each complete RM API/legality rule is implemented. ACATS coverage is
**— for every unit below**: the selected cases use the local REPORT adapter,
not upstream library tests. P8 owns the API-by-API audit; the owner column names
the feature follow-up. Missing units listed below are representative, not exhaustive.

| RM | Unit(s) | Implemented subset / restrictions and missing checks | Regression evidence | Roadmap owner |
| --- | --- | --- | --- | --- |
| A.1 | Standard (built in) | Scalar/string types, exceptions; wider characters pending; exact static evaluation is bounded (see above) | `arithmetic`, `integerliteral*`, `strings` | P5; P7 wide types |
| A.2 | Ada | Root namespace | `hello` | P8 |
| A.3.1–A.3.2 | Ada.Characters; Ada.Characters.Handling | Byte-character classification/conversion; wide facilities missing | `characterhandling` | P7 wide types |
| A.3.3 | Ada.Characters.Latin_1 | Latin-1 constants | `characterhandling` | P7 wide types |
| A.4.1 | Ada.Strings | String-operation declarations/exceptions | `fixedstrings` | P7 strings |
| A.4.2 | Ada.Strings.Maps | Mapping/set subset; callback operations incomplete; Constants child missing | `stringmaps` | P7 Maps.Constants |
| A.4.3 | Ada.Strings.Fixed | Fixed strings; byte components only | `fixedstrings` | P7 strings |
| A.4.4 | Ada.Strings.Bounded | Bounded strings; byte components only | `boundedstrings` | P7 strings |
| A.4.5 | Ada.Strings.Unbounded (missing) | Not implemented | — | P7 Unbounded |
| A.5 | Ada.Numerics | Root constants/exceptions | `numerics` | P8 |
| A.5.1 | Ada.Numerics.Generic_Elementary_Functions; Ada.Numerics.Elementary_Functions; Ada.Numerics.Long_Elementary_Functions | Host numeric functions; core arithmetic model documented, full library/Annex G accuracy audit remains | `runtime.numerics_precision` | P5 Real edge cases complete; P8 |
| A.7 | Ada.IO_Exceptions | I/O exception declarations | `textfiles`, `seqio`, `directio` | P8 |
| A.8.1 | Ada.Sequential_IO | Generic file I/O; representation-based elements | `seqio` | P7 streaming |
| A.8.4 | Ada.Direct_IO | Generic direct files; representation-based elements | `directio` | P7 streaming |
| A.10 | Ada.Text_IO | Text files and formatting subset; remaining grammar cases pending | `textfiles`, `filesnapshots` | P7 Text_IO |
| A.10.8 | Ada.Text_IO.Modular_IO | Unsigned 64-bit decimal/based file and string I/O; full Text_IO audit pending | `modulario`, mode/determinism and sanitizer checks | P5 Modular types; P7 I/O |
| A.10.8 | Ada.Text_IO.Integer_IO; Ada.Integer_Text_IO | Integer input/output subset | `typeio` | P7 numeric grammar |
| A.10.9 | Ada.Text_IO.Float_IO; Ada.Float_Text_IO | Real input/output subset; floating model audit pending | `floats`, `typeio` | P5 real edge cases; P7 |
| A.10.9 | Ada.Text_IO.Fixed_IO | Binary scales 2**(-62)..2**62, exact text conversion and local dynamic subtype instances; broader I/O audit pending | `fixed*` | P5 fixed point; P7 |
| A.10.9 | Ada.Text_IO.Decimal_IO | Exact decimal file/current-file/string I/O, formatting, parsing and subtype checks; full Text_IO audit pending | `decimalpoint`, `decimalchecks`, `decimaltasking`, mode/determinism and sanitizer checks | P5 Decimal fixed point; P7 I/O |
| A.10.11 | Ada.Text_IO.Enumeration_IO | Enumeration input/output subset | `enumio` | P7 Text_IO |
| A.12.1 | Ada.Streams | Representation-oriented stream support; full abstraction/attributes pending | `streamio`, `streamresults` | P7 streaming |
| A.12.1 | Ada.Streams.Stream_IO | File stream subset; type-aware/class-wide streaming pending | `streamio` | P7 streaming |
| A.15 | Ada.Command_Line | Arguments/environment exit status | `commandline`, `runtime.shared_tables` | P8 |
| A.18 | Ada.Containers families (missing) | Not implemented, including Ada 2005 indefinite containers | — | P7 containers |
| A.16–A.17 | Ada.Environment_Variables; Ada.Directories (missing) | Need host policies; Directories depends on Calendar.Time | — | P7 |
| 3.9 | Ada.Tags | Single-parent tag registry/dispatch subset | `tagged*`, `runtime.shared_tables` | P6f interfaces; P7 streams |
| 7.6 | Ada.Finalization | Controlled lifetimes/owned results; general limited results and controlled variant components pending | `controlled*` | P6a/P6c/P6e |
| 11.4.1 | Ada.Exceptions | Messages/occurrences/tracebacks; expanded/wide names, occurrence streaming/results missing | `exception*`, `savedoccurrences` | P7 Exceptions |
| 13.11.2 | Ada.Unchecked_Deallocation | Named object collections; cross-task allocation/deallocation rejected | `deallocation*`, `taskallocations` | P6a collections |
| 9.6.1 | Ada.Calendar and children (missing) | Not implemented | — | P7 Calendar |
| D.8 / Annex | Ada.Real_Time | 64-bit monotonic nanoseconds; host scheduling, no hard real-time guarantees | `time*`, `runtime.time_lifecycle` | P4 Time; P9 annex |
| C.7.1 / Annex | Ada.Task_Identification | Identity/status; Abort_Task missing | `taskidentity*` | P4 Task identity; P9 |
| A.3–A.4, A.10–A.11 | Wide/Wide_Wide character, string and I/O units (missing) | Source encoding and Ada 2005 identifier rules also need definition | — | P7 wide types |

`Ada.Text_IO.Scanning` is an **Extension**: an internal helper used by the numeric
I/O bodies, not a predefined Ada 2005 unit (`typeio`; P7 numeric grammar).

## Maintaining this baseline

For each feature, split/add the relevant RM/unit row, list remaining checks and
restrictions, attach executable and rejection regressions, update the ACATS
selection/expectations, and keep its roadmap owner current. Review completed
features as well as new work. Add implementation-defined choices and extensions
explicitly. Do not infer conformance from regression counts or selected ACATS
passes. Host/target, sanitizer and uninstrumented generated-access gaps remain
in Phase 8; abort/rendezvous/full tasking remain in Phase 9.
