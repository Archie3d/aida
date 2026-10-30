# Compiler improvement strategy

AIDA compiles a substantial, regression-tested Ada subset (see the README's
"Features and limits" section). This roadmap sets out how to grow it into a
dependable Ada 2005 compiler. It is ordered by risk: first remove ways a program
can be silently miscompiled, then lay the foundations tasking needs, harden what
already works, deliver a first tasking subset, and only then widen the language
and library.

Tasking is not left until last. It touches the run time, masters, elaboration,
exception state and the generated code's access to global state, and every
feature built on single-threaded assumptions makes it harder to retrofit.
The foundations and a minimal tasking slice therefore come before the large
language extensions, and the full tasking model follows them.

## Language target and scope

Ada 2005 is the normative language target for new work and remaining audits,
using the Ada 2005 Reference Manual for syntax, legality and dynamic semantics.
The near-term milestone is a dependable Ada 2005 subset, not full conformance.
The destination includes the unfinished Ada 95 foundations, Ada 2005 additions,
the core predefined environment and full core tasking (Phase 9).

Specialized Needs Annex support is tracked separately: individual facilities
below are selected goals, not commitments to complete annexes or a certified
Ravenscar implementation. Ada 2012 and later features are outside this target.
Completed items retain their historical audit baselines and subset restrictions;
they are not evidence of complete Ada 2005 coverage. Phase 8 tracks the gaps.

## Guiding principles

1. **Reject what is not implemented.** Accepting a construct and quietly
   dropping its meaning is worse than refusing it. Every unsupported construct
   gets a clear diagnostic until it is implemented.
2. **Prove every change.** Each behavior lands with a focused executable test in
   `tests/ada/`, a rejection test for each legality rule, and a determinism test
   when code generation changes. The full suite stays green. Mark an item
   complete only once its remaining restrictions are recorded here and in the
   README; completed entries stay brief, with details in the named tests and
   the commit history.
3. **Extend existing mechanisms.** Build on the array descriptors, owned-result
   ABI, finalization chains, tag descriptors and token-based generic instantiation
   rather than adding parallel paths.
4. **Correctness before speed.** Optimize only once tests pin down the existing
   failure behavior.
5. **Keep the README truthful.** Update its supported and unsupported lists in the
   same change that alters them.
6. **Design for tasking.** New run-time state belongs to the per-task context
   (Phase 2), never to a new C global. Generated code reaches mutable run-time
   state only through that context. Once Phase 4 lands, a feature that
   interacts with tasks gets a test that runs it from several tasks.
7. **Leave `qbe/` unchanged.**
8. **Run validation alongside implementation.** The Phase 5 prelude creates
   Phase 8's coverage matrix and ACATS harness; maintain both throughout the
   roadmap. Add relevant Ada 2005 ACATS coverage and matrix rows with each
   feature rather than waiting for the earlier phases to finish.
9. **Follow dependencies, not just bullet order.** Keep the phase sequence,
   but resolve local prerequisites before their dependents. In Phase 6,
   establish general limited-record semantics before generalizing limited
   aggregates and build-in-place returns; ordinary noncontrolled extended
   returns can proceed independently. In Phase 7, implement `Ada.Calendar`
   before `Ada.Directories`, whose modification-time operations use
   `Ada.Calendar.Time`, and wide character/string types before wide exception
   names and related library APIs. Shared access-type collections precede the
   Phase 7 library, whose allocating packages must work from several tasks.
   The abort-deferral design precedes Phase 6's finalization extensions, and
   record representation clauses follow discriminated records.
10. **Establish tasking's limited-type rules early.** Phase 4 must enforce the
  noncopying, lifetime and finalization rules needed by task/protected objects
  and composites containing them, without waiting for Phase 6's general
  limited-record support. Keep real concurrency in Phase 4 and full core
  tasking in Phase 9; the latter remains a language requirement, not an
  optional annex goal.

## Phase 1 — Stop silent acceptance

Complete. Constructs whose meaning was dropped now get diagnostics; later
phases replace each rejection as they implement it.

- [x] **Pragmas.** `Import` implemented; `Inline` advisory; `Pure` and
  `Preelaborate` enforced in Phase 7; other pragmas initially rejected
  (`pragmas`, `pragma*errors`). Elaboration,
  `Atomic` and `Volatile` pragmas were added in Phases 3–4.
  Assertion/check policies and No_Return were added in Phase 6h.
- [x] **Representation clauses.** Only validated `'Size` clauses accepted
  (`sizeclauses`, `representationerrors`, `size*errors`); Phases 5 and 6c add
  the other supported clauses.
- [x] **Imports.** C and Ada conventions, three positional arguments, nonempty
  external name (`importconvention*`).
- [x] **Stubs.** Initially diagnosed with recovery; subprogram/package
  subunits implemented in 6e (`subprogramstuberrors` retains illegal placements).
- [x] **Reserved words.** `aliased`, `protected`, `requeue`, `until`,
  `interface`, `synchronized` reserved; unsupported uses rejected (`reserved*`).
- [x] **Generic formal types.** Unsupported categories and trailing tokens
  rejected (`genericformal*errors`, `deallocationformalerrors`).
- [x] **Unsupported syntax.** Abstract subprograms, `select`,
  task entries, `limited with`, `private with`: named diagnostics with recovery
  (`unsupported*errors`).

## Phase 2 — Tasking foundations

Complete. Run-time and code-generation groundwork for threads, with no new Ada
syntax. Measurements: `tests/benchmarks/README.md`; cross-host validation moved
to Phase 8.

- [x] **Per-task context.** Exception state, trace frame and finalization/
  allocation registries in one C11 thread-local record; the environment task
  owns library storage (`runtime.task_context`, `runtime.array_storage`).
- [x] **Generated access to the context.** The traceback-entry call returns the
  context (QBE `thread` data is unusable for arm64 Linux externs and Windows
  amd64); no hidden parameter or TLS reference (`runtime.generated_contexts`,
  `determinism.generatedcontexts`).
- [x] **Result buffers.** Caller-owned storage for images, fixed-point text and
  stream results; signed 32-bit length limits remain (`imageresults`,
  `streamresults`, `runtime.result_buffers`).
- [x] **Shared run-time tables.** Per-file mutexes, locked tag registry, atomic
  exit status; no callbacks or blocking I/O under table locks
  (`runtime.shared_tables`, `runtime.blocking_io`, `filesnapshots`).
- [x] **Threading layer.** `adathread` over POSIX threads (winpthreads on
  MSYS2): threads, mutexes, monotonic waits, lock-free 8–64-bit atomics
  (`runtime.threading`).
- [x] **Masters and activation hooks.** `adamaster` brackets subprograms,
  blocks, collections and the library master; inside-out await/finalize/release
  on every exit; empty boundaries elided (`runtime.master*`,
  `determinism.masterhooks`).

## Phase 3 — Harden the existing subset

Complete. RM audits of supported features with focused regressions and
whole/separate/determinism tests. Remaining restrictions are listed per item
and in README.md.

- [x] **Unit identity and loading.** Per-declaration identities, separate
  spec/body nodes, canonical deduplication, automatic lookup and cycle
  diagnostics (`loading.*`, `ada.*_main`). Source bundles and custom filenames
  are extensions; generic subprogram renaming-as-body completions unsupported.
- [x] **Elaboration order and checks.** Binder validation of order and implied
  constraints; elaboration flags raise `Program_Error` on early calls or
  instantiations (`binding.elaboration_order`, `elaboration*`, `forward*`).
- [x] **Elaboration pragmas.** `Elaborate`, `Elaborate_All`, `Elaborate_Body`
  through manifests, binding and incremental rebuilds (`elaboration.pragmas`,
  `pragmaelaborationerrors`, `ada.ep_main`).
- [x] **Visibility and conformance.** Homographs, use/direct visibility,
  private/body isolation, full spec/body conformance, missing bodies diagnosed
  before linking (`duplicate*errors`, `profile*`, `visibility*`,
  `requiredcompletionerrors`). Library package bodies without `Elaborate_Body`
  remain optional.
- [x] **Blocks during package-body elaboration.** Per-unit elaboration frame
  with scoped cleanup (`elaborationblock*`).
- [x] **Aggregates.** Shared checked shape/choice/sliding logic; record
  aggregates via a checked temporary (`aggregateaudit*`, `aggregatecleanup`).
  Box defaults added in 6b.
- [x] **Array conversions.** Per-dimension checks and sliding; writable actuals
  evaluate once and copy back on normal return (`arrayconversion*`,
  `controlledconversions`). Identical component subtypes required; limited
  controlled array conversions unsupported.
- [x] **Derived types.** Untagged conversions, inherited constraints/profiles,
  derived Boolean/character, shared access collections (`derived*`). Writable
  non-array untagged conversions, new discriminant parts, derivation before
  private completion and wide character components remain diagnosed.
- [x] **Attributes.** `'Pos`, `'Val`, `'Succ`, `'Pred`, `'First`, `'Last`,
  `'Range` with single prefix evaluation (`attributeaudit*`, `attributebounds`).
- [x] **Primitive operations.** Overriding indicators, hiding, subtype
  conformance, controlling defaults/results, freezing by objects and bodies
  (`primitiveaudit*`, `primitiveplacementerrors`). Tagged discriminants,
  private extensions and general freezing remain open.
- [x] **Variant records.** Nested variants through layout, aggregates,
  initialization, selection and equality (`nestedvariants*`). Runtime and
  mutable discriminants and controlled variant components added in 6c.
- [x] **Feature interactions.** Combined overloads, instances, defaults,
  conversions, controlled ownership and exceptions, checking copy-back and
  cleanup on every exit (`interaction*`, `semantics.interaction_modes`).

## Phase 4 — Minimal tasking

Complete. A tested near-Ravenscar subset without claiming compliance; Phase 9
completes core tasking. Each item has whole/separate/determinism tests and
passes TSan/ASan/UBSan on the C run time (QBE code is uninstrumented, see
Phase 8). RM: 3.11, 7.5, 7.6.1, 9.1–9.10, C.6, C.7.1, D.8.

- [x] **Tasks.** Entry-free task types and single tasks, grouped activation,
  masters joining dependents, task components and allocated tasks (`tasks`,
  `taskallocations`, `tasklibrarymain`, `taskactivationfailure`, `task*errors`,
  `runtime.tasking*`). Not copyable; discriminants, entries, tagged/variant task
  composites, aggregates and function results diagnosed.
- [x] **Protected objects.** Procedures and functions under one lock per object
  (`protected*`, `runtime.protected_lifecycle`). Not copyable; discriminants,
  interfaces, operation renaming, named access and generic actuals diagnosed.
  External self-calls and task creation in an action raise `Program_Error`.
- [x] **Protected entries.** Single entries with Boolean barriers, FIFO queues,
  `'Count`, copy-back after release (`protectedentries`, `entry*`,
  `runtime.entry_lifecycle`). Barrier exceptions raise `Program_Error` in
  queued callers; families, select and requeue diagnosed.
- [x] **Time.** 64-bit `Duration` (`Small = 2**(-30)`), `delay`, `delay until`,
  `Ada.Real_Time` on the monotonic clock (`time*`, `runtime.time_lifecycle`).
  No hard real-time guarantees; delays in protected actions raise
  `Program_Error`.
- [x] **Shared variables.** `Atomic`/`Volatile` scalar objects and types via
  sequentially consistent run-time accesses (`shared*`, `semantics.shared_ir`).
  Shared composite parts and component pragmas diagnosed; aligned RAM only.
- [x] **Task identity.** `Ada.Task_Identification`, `'Identity`, `'Callable`,
  `'Terminated`, entry `'Caller`; `Current_Task` in an entry body returns the
  caller (`taskidentity*`). `Abort_Task` waits for Phase 9.

Task entries, select/requeue, abort and Ravenscar compliance remain for Phase 9.

## Phase 5 — Numeric and representation foundations

Complete for the documented subsets. Remaining gaps keep their later-phase
owners in the coverage matrix. Unless noted, items have whole/separate/
determinism tests and instrumented C run-time checks.

### Prelude

- [x] **Validation baseline.** Coverage matrix skeleton
  (`docs/ada2005-coverage.md`) and offline CTest ACATS harness with pinned
  XFAIL/XPASS detection (`acats.*`, `tests/acats/README.md`). The local REPORT
  adapter supports only C-tests; B-tests and the full support protocol remain
  open. Phase 8 maintains both.
- [x] **Lowering decision.** Keep direct QBE emission with explicit layout,
  call/result and scope-exit contracts
  ([decision and migration gates](docs/lowering-decision.md)).

### Numeric and representation work

- [x] **Static arithmetic.** Checked exact folding separate from machine
  arithmetic (`staticarithmetic*`, `integerliteral*`, ACATS C49022A). Bounded
  to 127-bit integer/rational and 64-bit scaled fixed intermediates; arbitrary
  precision and the full RM staticness audit remain open
  ([limits](docs/static-arithmetic.md)).
- [x] **Real edge cases.** Checked binary32/64 arithmetic and Integer-exponent
  powers; zero divisors, nonfinite results and narrowing overflow raise
  `Constraint_Error` (`realedges`, `realtasking`, `realedgeerrors`,
  `runtime.real_arithmetic`, ACATS C45523A). Static-capacity, model-attribute
  and Annex G restrictions: [floating model](docs/floating-point.md).
- [x] **Floating-point `'Value`.** Locale-independent decimal/based parsing,
  rounded once to binary32/64 (`realvalue*`, `runtime.real_value`).
  [Contract](docs/floating-point.md#floating-value).
- [x] **Array descriptors.** ABI v2: signed 64-bit bounds, lengths and byte
  strides with checked size arithmetic for all array views
  (`arraydescriptor*`, `arrayframeerrors`, `runtime.array_descriptors`). Old
  caches require rebuilding. [Contract](docs/array-descriptors.md).
- [x] **Concatenation.** General nonlimited one-dimensional `&`, including
  wide, private and controlled components (`concatenation`, `concattasking`,
  `concaterrors`, `runtime.concatenation`, ACATS C45347A).
  [Limits](docs/array-descriptors.md#concatenation).
- [x] **Modular types.** Through `mod 2 ** 64`, modular formals, `'Mod`,
  `Modular_IO` (`modular*`, `runtime.modular`, ACATS C354002). Index subtypes
  and variant choices keep signed 64-bit bounds; compact unsigned 32-bit
  storage unsupported. [Limits](docs/modular-types.md).
- [x] **Ordinary fixed point.** Binary scales `2.0**(-62)`–`2.0**62`, Small
  clauses, dynamic local bounds, Ada 2005 fixed-by-fixed rules, fixed streams
  (`fixedwide*`, `fixedoperator*`, `fixedstreams`, `fixedtasking`,
  `runtime.fixed_arithmetic`, ACATS C35A08B). [Limits](docs/fixed-point.md).
- [x] **Decimal fixed point.** Scales `10.0**(-18)`–`10.0**18`, 1–18 digits,
  truncating arithmetic, Round, decimal formals and Decimal_IO (`decimal*`,
  `runtime.decimal`, ACATS CXF2002/CXF2003).
  [Limits](docs/decimal-fixed-point.md).
- [x] **Representation clauses** ([contract](docs/representation-clauses.md);
  record clauses landed in 6c).
  - [x] Alignment: 0 and powers of two through 8, inherited, padded composite
    storage (`alignmentclauses`, `alignmenterrors`). Weakening natural
    alignment, stride padding, object clauses and tagged/task/protected types
    unsupported.
  - [x] Enumeration: static increasing signed 32-bit codes, 8/16/32-bit
    storage, derived mappings (`enumrepresentation`, `enumstreams`,
    `enumtasking`, `enumrepresentationerrors`, `runtime.enumeration`, ACATS
    CD3014A/CD3014D). Boolean/Character-derived mappings, wider codes and
    clauses after a private-view subtype unsupported.
  - [x] Size-changing scalar subtypes and byte-aligned composite layouts
    (`sizestorage`, `sizestreams`, `sizetasking`, `sizestorageerrors`,
    `semantics.type_storage`). Named-subtype Size clauses are an AIDA
    extension. Bit packing, dynamic layouts and nonconfirming tagged/
    controlled/limited/task/protected sizes diagnosed; ACATS CD2A22A is a
    packing XFAIL.
- [x] **Checked arithmetic speed.** Inline signed 32/64-bit checks for all
  integer operators with unchanged failure paths; 88,000 differential cases per
  mode (ACATS C45504A/C45611A). [Contract](docs/checked-arithmetic.md),
  [measurements](tests/benchmarks/README.md#checked-integer-arithmetic).
- [x] **Named operator calls.** Left/Right associations for predefined and user
  operators (`namedoperators`, `namedoperatorerrors`, ACATS C45503A).

## Phase 6 — Complete the core language

Complete the remaining Ada 95 foundations and Ada 2005 additions together.
Grouped by dependency into milestones 6a–6h, taken in order; within a group,
work top to bottom, except that the first three 6e items come before the
remaining 6d items. Run 6f-prelude after 6e and before starting 6f.
The 6h pragmas have no prerequisites here and may be pulled forward. Reuse
the existing controlled-result construction, cleanup and dispatch mechanisms. Completed items have
whole/separate/determinism tests and, where relevant, four-task and sanitizer
checks.

### 6a. Tasking prerequisites

- [x] Abort-deferral foundation: task-owned nesting counter, balanced across
  protected actions, entry servicing and finalization paths; explicit call
  effects and return insertion points (`abortdeferral`,
  `runtime.abort_deferral`). Abort delivery, blocking cancellation, deferred
  Initialize/controlled assignment and master waits remain Phase 9.
  [Design](docs/abort-deferral.md).
- [x] Shared access-type collections: synchronized allocation, deallocation and
  heap finalization across tasks; collection tasks belong to the access type's
  master (`sharedcollections`, `sharedtaskcollections`,
  `runtime.shared_collections`; ACATS C761002 PASS after P6e labels/renaming).
  Foreign stack chains and custom pools unsupported.
  [Limits](docs/shared-collections.md).

### 6b. Dynamic data

- [x] Runtime scalar constraints: captured local/library bounds, real bounds,
  runtime Width and checked scalar streams, reused by generics, aliases and
  calls (`scalardynamic`, `scalarstreams`, `scalartasking`,
  `scalardynamicerrors`, `runtime.scalar_bounds`, ACATS C35507A). Deferred
  completions need static matching; explicit scalar allocator constraints are
  illegal. [Limits](docs/runtime-scalars.md).
- [x] Library-level arrays and types with runtime bounds, captured during
  elaboration (`runtimearraylibrary*`, ACATS C36203A).
  [Limits](docs/array-descriptors.md#library-level-runtime-bounds).
- [x] Runtime-constrained array components and allocators, with Size and
  all-dimensional streams (`runtimecomponent*`, `runtimeallocators`,
  `runtime.dynamic_layout`, ACATS C48005B). Runtime-sized tagged records and
  delayed private-component layouts are completed in 6f.
  [Limits](docs/runtime-composites.md).
- [x] Ada 2005 box defaults (`<>`) in record and array aggregates, with checked
  subtypes and partial cleanup (`boxaggregates`, `boxchecks`, `boxcleanup`,
  `boxtasking`, `aggregateboxerrors`, `boxpositionalerrors`). Task/protected
  aggregates remain Phase 9; general limited records landed with 6e returns.
  [Limits](docs/aggregate-defaults.md).

### 6c. Discriminated records

- [x] Default discriminants, constrained/unconstrained objects, mutable
  untagged variants and `'Constrained` (`defaultdiscriminants*`, ACATS
  C37206A/C37306A). [Limits](docs/default-discriminants.md).
- [x] Runtime discriminant constraints and discriminant-dependent components
  (`runtimediscriminants`, `discriminantcomponents`, `discriminantcleanup`,
  `discriminanttasking`, ACATS C37211A/C37108B). Dependent controlled/task/
  protected arrays and access-subtype constraints retain diagnostics; tagged
  composites and managed component streaming landed in their later milestones. [Limits](docs/runtime-discriminants.md).
- [x] Controlled components in variant records, with partial-failure cleanup
  (`controlledvariants`, `controlledvariantlimited`; ACATS C760007 PASS,
  C761012 XFAIL for new derived tagged discriminants). Task/protected variants
  diagnosed. [Limits](docs/controlled-variants.md).
- [x] Record representation clauses: byte-addressable positions, narrowed
  scalars, variant overlays, bounded dependent arrays (`recordrepresentation`,
  `recorddependent`, `recordcontrolled`, `recordrepresentationerrors`; ACATS
  CD4051A/CD1C03H PASS, CD4031A packing XFAIL). Bit packing, unaligned fields,
  changed inherited layouts and general freezing are Phase 8; tagged layouts
  6f. [Contract](docs/representation-clauses.md).

### 6d. Access types

Complete for the documented access subsets. Broader type definitions, generic
formals, protected callback types and foreign ABI work retain their later owners.

- [x] General access types: `aliased`, `access all`, `'Access`,
  access-to-constant, accessibility and checked named conversions
  (`generalaccess*`). Dependent-component checks are conservative; limited
  current instances are completed in 6f; ACATS C3A0013 now PASS after the Phase 7
  stream-array Range syntax prerequisite. [Contract](docs/general-access.md).
- [x] Anonymous access definitions in object, component, discriminant,
  parameter, result, extended-return and renaming contexts, with master-ancestry
  checks, coextensions, protected callbacks and controlling access dispatch
  (`anonymousaccess*`, `anonymouscoextensions`, `anonymousprotected`,
  `anonymousdispatch`; ACATS C460001/C460002/C3A2002). Inline anonymous array
  objects (`anonymousarrayobjects`, `anonymousarrayerrors`). Broader limited/
  tagged definitions, generic formals and task/protected discriminants remain
  6f/6g/9. [Contract](docs/anonymous-access.md).
- [x] Null exclusions (`not null`): profile conformance, initialization,
  conversions, generic subprogram matching and runtime checks
  (`nullexclusion*`, ACATS C3A0030 PASS; C3A0020/C3A0025 now PASS after
  6g access formal objects; limited records landed in 6e). [Contract](docs/null-exclusions.md).
- [x] `'Access` of imported Ada/C subprograms and inherited tagged operations,
  with calling-convention checks, native C callbacks, inherited entries and
  result tags (`importedcallback*`, `inheritedcallback*`, `callbackextension*`;
  ACATS C3A0006 PASS, C3A0009 private-extension XFAIL).
  [Contract and remaining ABI limits](docs/subprogram-access.md).

### 6e. Program structure and names

The first three items are cheap and unblock common Ada 2005 code and ACATS
cases (C59002A, C761002, part of C3A0013); take them next.

- [x] Labels and `goto`, with legality checks for transfers between scopes.
  Forward/backward and expanded label names; statement identifier uniqueness,
  visibility and sequence/body boundaries checked. Outward transfers reuse
  master/finalization/storage cleanup (`gotostatements`, `gotocleanup`,
  `goto*errors`, whole/separate/determinism; ACATS C51004A/C59002A/C59002B PASS).
- [x] Package and generic renaming: new views share package declarations,
  type/object identity and generic contracts, including alias chains and current
  package instances. Library alias loading, completion/child restrictions,
  privacy, defaults and elaboration checked (`unitrenaming*`, `renaminglibrary`,
  `renaming*errors`, `semantics.unit_renaming`; whole/separate/determinism).
  Root `Unchecked_Deallocation` compatibility view reuses the Ada child unit;
  unchanged ACATS C85011A and C761002 PASS. Generic subprogram renaming-as-body
  completions retain the existing Phase 3 limitation (Phase 8 audit).
- [x] Prefixed calls (`Object.Operation (...)`) on tagged types: inherited
  primitives, class-wide dispatch/operations, access prefixes/formals, defaults
  and overload resolution (`prefixedcalls`, `prefixedcleanup`, `prefixed*errors`;
  whole/separate/determinism, four workers; ACATS C413001–C413004 PASS).
  [Contract](docs/prefixed-calls.md). Interfaces follow in 6f; prefixed-view
  renaming/generic actuals remain diagnosed for the Phase 8 audit.
- [x] Limited/private `with`, including `limited private with`: distinct
  incomplete/tagged views, private/child visibility and completion checks;
  limited graph/manifest edges preserve mutual dependencies without imposing
  elaboration order (`limitedwith`, `privatewith`, `taggedincomplete`,
  `semantics.context_views`, `loading.context_dependencies`; whole/separate/
  determinism, four tasks). ACATS CA20003/CA110232 PASS; CA120012 now PASS
  with limited records and CA12002 PASS with subunits below.
  Existing full-type restrictions remain; [contract](docs/context-views.md).
- [x] Object `Unchecked_Access` (pulled forward from the Phase 8 access audit):
  library-level accessibility, including anonymous access parameters and local
  tags; ordinary aliasing, subtype, constness and storage rules retained.
  `uncheckedaccess`, `uncheckedaccesserrors`, `semantics.unchecked_access`;
  whole/separate/determinism and four tasks. CA110232/C3A2004 PASS after the
  callback follow-up below. [Contract](docs/unchecked-access.md).
- [x] `'Access` of dereferenced access-to-subprogram values (pulled forward
  from Phase 8): contextual overloaded function results, single evaluation,
  null checks and captured-frame identity; profile, convention, privacy and
  accessibility legality retained. `callbackdereference*` covers whole/separate/
  determinism and four tasks; unchanged ACATS C3A2004 PASS.
  [Contract](docs/subprogram-access.md#dereferenced-callbacks).
- [x] Separate subunits for subprogram and package bodies: recursive loading,
  stub-scope visibility, local contexts, profile conformance and generic replay;
  parent object dependencies include transitive subunit sources (`subunits`,
  whole/per-unit/determinism and four tasks; `semantics.subunits`,
  `subprogramstuberrors`). ACATS CA1006A/CA2009D/CA12002 PASS; CA2009A was
  promoted in 6g after retaining constrained-array actual subtypes. Task/protected subunits
  remain diagnosed. [Contract](docs/subunits.md).
- [x] Noncontrolled extended returns, general limited aggregates and
  build-in-place results for supported result types. Pulled forward explicitly
  limited record full views from 6f; preserve intrinsic limitedness through
  subtypes/completion/derivation, identity, constraints, accessibility and
  master/cleanup paths (`extendedreturns`, `limitedresults`, `returncleanup`,
  whole/per-unit/determinism and four workers; `semantics.returns`,
  `runtime.array_storage`). ACATS C650003/CA120012 PASS; C3A0025 now PASS
  after 6g access formal objects. Task/protected aggregates/results remain
  diagnosed for Phase 9. [Contract](docs/returns.md).

### 6f-prelude. ACATS gaps in supported features

Repair correctness and legality gaps exposed by the
[completed-feature ACATS baseline](docs/acats-baseline.md) before adding the
6f language features. Localize each failure and confirm the applicable Ada
2005 rule before changing the compiler. This milestone covers the existing
supported subset; documented private-extension, generic-formal, library and
full-tasking restrictions retain their later roadmap owners.

- [x] **Constrained class-wide views and tagged variants.** C390010 now PASS
  unchanged. Preserve captured discriminant constraints through `'Class` and
  subtype aliases, check access/value boundaries, and select independent parent/
  extension variants by their own discriminants (including `others`). Initialize
  each ancestor's discriminants after the child discriminants they reference.
  `classwideconstraints` and `taggedvariantgroups` cover static/runtime defaults,
  nullable/anonymous access, membership, calls, results and failed assignments
  in whole/per-unit/deterministic modes with four workers.
  Full suite 1,137/1,137 passes; both fixtures pass with the compiler and C
  runtime checked separately under ASan/UBSan. C390010 also compiles with the
  instrumented compiler; all other 121 baseline observations remain unchanged.
- [x] **Derived scalar operations.** C34001A/C34001D/C34002A now PASS unchanged:
  scalar `'Value` returns/checks `S'Base`, including runtime-constrained derived
  types (RM 3.5(53)). `derivedscalaroperations` covers enumeration, Boolean and
  integer operations, inherited constraints, conversions and receiving-context
  checks in whole/per-unit/deterministic modes and four tasks.
- [x] **Controlled adjustment failure.** C761006 now PASS unchanged: disjoint
  assignment adjusts the actual target and completes remaining adjustments
  before `Program_Error`; overlapping views retain owned snapshots.
  `controlledadjustment` covers records/arrays/class-wide objects, parent and
  component failures, partial cleanup/recovery and deallocation in whole/per-unit/
  deterministic modes and four tasks; runtime ASan/UBSan and allocation balance
  checks pass. [Contract](docs/controlled-assignment.md).
- [x] **Primitive operator visibility.** C840001/C84008A/C84009A PASS unchanged:
  implicit operators obey scope hiding and package-qualified lookup; `use type`
  follows type identity. `operatorvisibility` and `operator*visibilityerrors`
  cover homographs, subtypes, named/expanded calls, private/body/child contexts
  and rejection controls; whole/per-unit/determinism, four tasks and compiler
  ASan/UBSan pass. B840001 stays ungraded. [Contract](docs/operator-visibility.md).
- [x] **Array declaration legality.** Shared array parsing rejects mixed
  constrained/boxed indices for named types, anonymous arrays and generic
  formals (RM 3.6(2–6)). `arrayindexerrors` covers both orders and recovery;
  `arrayindexlegality` supplies constrained/boxed, runtime, null and generic
  legal controls in whole/per-unit/deterministic modes; compiler ASan/UBSan
  pass. Unchanged B36002A now gets 12 targeted diagnostics and remains
  `B_REJECTED_UNGRADED` until Phase 8 validates error markers and legal controls.
- [x] **Expanded names of enclosing declarations.** C83022A/C83024A and related
  C64004G/C64005A/C64005B PASS unchanged. Enclosing blocks/loops/subprograms
  qualify their immediate declarations; generic current-instance and captured
  identities survive replay. `enclosingnames`/`enclosingnameerrors` cover hiding,
  defaults, overloads, type/exception names and statement-prefix/value legality
  in whole/per-unit/deterministic modes and four tasks; compiler ASan/UBSan pass.
  [Contract](docs/enclosing-names.md).
- [x] **Case-selector overload resolution.** C540002 PASS unchanged: discover
  visible discrete interpretations before binding; choices cannot disambiguate
  selectors. `caseselector`/`caseselectorerrors` cover direct/qualified/operator/
  generic calls, subtype preservation, single evaluation and genuine ambiguity
  in whole/per-unit/deterministic modes and four tasks; compiler ASan/UBSan pass.
  [Contract](docs/case-selectors.md).
- [x] **Real named-number arithmetic.** C49022C PASS unchanged: resolve the
  mixed root real/integer multiplying profiles through the checked exact
  evaluator; converted named numbers retain universal numeric types.
  `realnumbers`/`realnumbererrors` cover mixed/nested arithmetic, conversions,
  rounding, fixed contexts, invalid operands, static checks and capacity limits
  in whole/per-unit/deterministic modes and four tasks; compiler ASan/UBSan pass.
  [Contract](docs/static-arithmetic.md).
- [x] **Remaining supported-area rejections.** Reviewed every remaining legal
  C rejection/support blocker; [audit and owners](docs/supported-rejections.md).
  C41103A/C41105A/C730004/C73002A PASS unchanged after these repairs:
  - [x] Resolve parameterless array/access-array function prefixes alongside
    ordinary calls, using index/component contexts and preserving ambiguity.
  - [x] Preserve slice nominal bounds when lowering nested indexing.
  - [x] Give nested package bodies the enclosing body's lexical declarations
    without exposing body locals to specification clients.
  - [x] Use nonlimited full views for concatenation/equality while preserving
    limited partial views and genuinely limited components.
  - [x] Parse class-wide/base subtype marks in use-type clauses (C760011 still
    needs P6f tagged discriminants and a P8 REPORT adapter).
  - [x] Select discriminants declared in visible private partial views
    (C37006A still needs P8 access-subtype discriminant constraints).
  `supportedrejections`/`supportedrejectionerrors` cover these repairs with
  whole/per-unit/determinism, four tasks and compiler ASan/UBSan checks.
- [x] **Validation and promotion.** All repairs have focused legal/rejection
  regressions and unchanged upstream checks; whole/per-unit/determinism,
  four-task execution and compiler ASan/UBSan pass. Final suite: 1,039/1,039.
  The reviewed 106-case snapshot has 55 executable PASS cases in the strict
  suite, 24 legal-source rejections, seven support blockers and 20 ungraded
  B observations. Coverage, exact observations, restrictions and concrete
  later-phase owners are updated in the [audit](docs/supported-rejections.md).
  The supported-area review is complete; this prelude is closed.

### 6f. Object orientation

- [x] Abstract subprograms and legality for supported profiles: inheritance,
  required overrides, dispatch-only calls, defaults, private abstract views,
  equality, completions and forbidden uses (`abstractsubprograms`,
  `abstractinheritance`, `abstract*errors`; whole/per-unit/determinism and four
  workers). ACATS C460006 PASS unchanged; B840001 remains ungraded.
  Compiler ASan/UBSan and full suite 1,050/1,050 pass.
  [Contract and boundaries](docs/abstract-subprograms.md).
- [x] Ada 2005 overriding indicators and null procedures for supported profiles:
  immediate/deferred checks, bodies, renamings, instances and protected
  declarations; null completion/conformance, incomplete views and early calls.
  Controlling access results dispatch through operands, factories and dereferenced
  class-wide assignments, with nullable results, accessibility and required
  overrides even for null extensions (`overridingnull`, `controllingaccessresults`,
  legality controls; whole/per-unit/determinism and four workers). Compiler
  ASan/UBSan checks and full suite 1,067/1,067 pass; all 106 baseline observations
  remain unchanged (56 executable PASS).
  [Contract and remaining boundaries](docs/overriding-null-procedures.md).
- [x] Discriminated tagged types, private extensions, extensions of completed
  private views, unknown-discriminant private views and static tagged Size
  padding. Runtime tagged layouts and delayed private-component layouts from
  6b now capture at elaboration. Dependent tagged arrays own separate component
  storage, including controlled/class-wide copies and ancestor assignments.
  Limited current instances support Access in defaults/access-discriminant
  constraints with object accessibility (`taggedviews`, `taggedruntime`,
  `taggeddependent`, `taggedcontrolled`, `taggedinstances`, `privatedelayed`,
  legality controls and the promoted `runtimecomponentlate` execution test).
  ACATS C3A0009/C761012 and baseline C3A0007/C432001/C432004/C760009 PASS
  unchanged; C3A0013 now PASS after the Phase 7 Range syntax prerequisite.
  Baseline: 60 PASS, 18 compile rejections, eight harness blockers, 20 ungraded
  B observations. Full suite 1,092/1,092; 17 focused compiler ASan/UBSan
  checks, seven whole/per-unit scenarios with the ASan/UBSan C runtime, and
  runtime ownership/layout checks pass.
  [Contract and boundaries](docs/tagged-discriminants.md).
- [x] Interfaces: multiple progenitors, transitive/diamond inheritance, shared
  operation slots, inherited concrete/null implementations and overriding;
  checked class-wide/access conversions, dispatch and Ada.Tags queries.
  Tags retain canonical object addresses and own interface maps/lexical links,
  including private contract entries, generic instances and controlled layouts.
  Ordinary/limited implementations and synchronized/task/protected contract
  categories have execution/legality coverage (`interfaces`, `interfacediamonds`,
  `interfaceownership`, `interfacecategories`, `interfaceprofiles`,
  `interfacegenerics`, `interface*errors`, runtime maps/failure injection).
  Whole/per-unit/determinism and four workers pass; full suite 1,115/1,115,
  16 compiler ASan/UBSan checks and six scenarios with the instrumented C
  runtime pass. All 106 ACATS baseline observations remain unchanged (60 PASS).
  Concurrent task/protected implementations retain Phase 9, with compatible
  slot-map metadata and explicit diagnostics until implemented.
  [Contract and boundaries](docs/interfaces.md).

- [x] Phase 6f ACATS backfill: 16 unchanged C-tests and their original shared
  foundations. Fifteen new executable passes cover interfaces, abstract/private
  dispatch, discriminated/local extensions and generic-instance tags. C390010
  now also passes after the constrained class-wide/variant repair in 6f-prelude.
  All original 106 observations remain unchanged. Expanded baseline: 122 cases,
  76 PASS, 18 compile rejections, eight harness blockers and 20 ungraded B
  observations. [Inventory and triage](docs/acats-baseline.md).

### 6g. Generics

Code: `Parser::parseGenericDeclaration`, `Sema::bindGenericFormals`,
`Sema::analyzeGenericInstantiation`.
[Implemented contract and limits](docs/generic-types.md).

The entries below expand the original five commitments: formal type
categories; formal subprograms and array matching; formal packages; formal
objects; and generic contract checking. They are implementation substeps, not
new feature families or equally sized measures of progress. Completed entries
record supported subsets, not completion of every case in a formal category.
Follow the listed order among unblocked items. Record newly discovered gaps
with evidence and a phase owner before starting an additional increment.
Phase 8 audits and Phase 9 concurrent execution remain separately owned below.

- [x] Constrained-array actual subtypes for private formals retain static and
  runtime bounds. Unchanged CA2009A executes as PASS.
- [x] Unknown-discriminant private/limited-private formals and named object/
  subprogram access formals. Match designated subtypes, general/constant access,
  null exclusions and mode-conformant callback profiles; check contracts before
  instantiation and replay declaration-site bindings. `genericaccess*`,
  `genericprivatearrays` and `genericprivateerrors`; CC54001–CC54003 PASS.
- [x] Derived formals for supported scalar, array, record and object-access
  types, plus tagged/abstract private formals. Preserve ancestry, constraints,
  definiteness, primitive contracts and public views through nested/library
  instances. `genericderived*`; unchanged C393013 (full null-extension
  controlling-result scenario), C3A2A01, C640001, C760010 and CC54004 PASS.
- [x] Interface formals with exact ordinary/limited/synchronized/task/protected
  categories; ordinary/limited interface ancestors and progenitors on derived
  formals. Preserve interface-origin operation mapping, copied contracts and
  abstract obligations. `genericinterface*` covers categories, ancestry,
  callbacks, reordered maps, diamonds and library bodies. B394A01/B394A02 are
  pinned ungraded rejection probes, not executable or legality passes.
- [x] Known discriminants on private/limited-private/tagged-private formals
  and nonstatic Definite on formal indefinite subtypes. Match visible,
  unconstrained actual discriminants by count/subtype; replay selections and
  named constraints by position through nested/library instances. Record
  membership checks discriminant constraints. `genericdiscriminant*` and
  `genericdefinite*`; unchanged CC1204A/CC3240A PASS.
- [x] **Synchronized formal derived extensions.** Parse and enforce
  synchronized ancestors/progenitors, category/limitedness rules and matching
  of actual descendants; preserve corresponding primitive operations through
  instances. Formal synchronized/task/protected interface declarations alone
  do not satisfy this item. Complete syntax, matching and contract tests here
  (RM 12.5.1(3/2–5.1/2)); concrete task/protected implementation and execution
  tests belong to Phase 9 and do not block completion of this static support.
  `genericsynchronized*` covers abstract/specific contracts, synchronized actual
  categories, ancestry, callbacks, unknown discriminants, nested/subtype/library
  replay, whole/per-unit/determinism and four workers. Full suite 1,227/1,227;
  14 compiler ASan/UBSan checks and three scenarios with the instrumented C
  runtime pass. Unchanged B394A03–B394A05 add ungraded probes; B394A02 loses two
  obsolete parser diagnostics, while the other 124 prior observations remain
  unchanged. The 128-case cohort has 79 PASS, 16 compile rejections, eight
  harness blockers and 25 ungraded B observations.
  [Contract](docs/generic-types.md#synchronized-derived-formals).
- [x] **Class-wide actuals for formal derived types.** Match ancestry,
  progenitors and definiteness; dispatch intrinsic copied operations with the
  actual tag, including controlling access parameters and results. Constructors
  without controlling parameters raise `Program_Error` for a statically formal
  tag, and all calls through their renamings raise it. Nested/library replay,
  whole/per-unit/determinism and four workers: `genericclasswide*`.
  Unchanged CC51B03 with FC51B00 executes as PASS.
  [Contract](docs/generic-types.md#class-wide-derived-actuals).
- [x] **Abstract formal subprograms.** Procedures/functions with exactly one
  specific tagged controlling type; mode-conformant dispatching actuals,
  declaration-site named defaults and box defaults. Preserve actual subtypes,
  formal names/defaults and dispatch slots through nested/library replay,
  including interface/access operands, controlling results and operators.
  Static calls, nonprimitive actuals, invalid profiles/defaults and abstract
  `Access` are diagnosed. `genericabstract*` runs whole/per-unit/determinism
  with four workers; unchanged CC60001 executes as PASS.
  [Contract](docs/generic-types.md#abstract-formal-subprograms).
- [x] **Null procedure defaults.** `is null` on concrete formal procedures;
  omitted actuals bind an intrinsic null procedure with the formal profile.
  Explicit replacements keep normal matching and convention. Calls retain
  argument/default evaluation, checks and cleanup; renamed/forwarded intrinsic
  views reject `Access`. Concrete formals hide inherited homographs without
  adding primitive slots. `genericnull*` runs whole/per-unit/determinism with
  four workers; unchanged BC60001 is an ungraded rejection probe, with all
  previous 128 baseline observations unchanged.
  [Contract](docs/generic-types.md#null-procedure-defaults).
- [x] **Attribute subprogram actuals.** Scalar Image/Value/Succ/Pred/Min/Max,
  with mode/profile checks, base-subtype profiles, Intrinsic convention and
  explicit/named/box-default binding through nested/library instances.
  Attribute defaults are checked before an instance; prefix bindings survive
  shadowing. Scalar Min/Max lowering and nonstatic formal scalar bounds cover
  the required prerequisites. `genericattribute*` runs whole/per-unit/
  determinism with four workers; unchanged CC1302A executes as PASS. CC3601A
  remains a compile XFAIL for grouped formal objects (Phase 8). Anonymous
  universal profiles cannot match named formals; standard stream profiles were added in
  the Phase 7 streaming milestone.
  [Contract](docs/generic-types.md#attribute-subprogram-actuals).
- [x] **Enumeration-literal subprogram actuals.** Identifier and character
  literals, including case-sensitive expanded names such as `Pkg.'A'`, bind
  parameterless functions with explicit/named/box binding. Named defaults retain
  declaration-site bindings; inherited result subtypes and Intrinsic convention survive nested/library
  replay and forwarding. `genericliteral*` covers whole/per-unit/determinism
  with four workers and invalid profiles/defaults/Access. Static related-enum
  conversions complete unchanged C540001, now an executable PASS; the other
  128 baseline observations are unchanged.
  [Contract](docs/generic-types.md#enumeration-literal-subprogram-actuals).
- [x] **Nonstatic constraints in formal array matching.** Shared nonstatic
  index ranges/subtypes and component constraints statically match by identity,
  including aliases, derived arrays and named index constraints. Per-axis
  constraint metadata survives nested/library replay independently of runtime
  descriptors; unrelated equal-valued constraints still reject. Three
  `genericarray*` scenarios run whole/per-unit/determinism with four workers;
  targeted rejections and compiler/C-runtime ASan/UBSan checks pass.
  All 1,300 tests pass; all 129 baseline observations remain unchanged.
  [Contract](docs/generic-types.md#nonstatic-constraints-in-formal-array-matching).
- [x] **Formal packages and contract matching.** Fully boxed views and
  complete positional/named/defaulted lists; template provenance, static subtype/
  value matching and entity identity survive renaming, forwarding and nested/
  library replay. Distinct views keep independent types and private declarations
  stay hidden. Formal-part use clauses retain source order; matching emits no
  duplicate elaboration. Four local scenarios run whole/per-unit/determinism
  with four workers; three rejection tests and compiler/C-runtime ASan/UBSan
  checks pass. Unchanged CC70002, CC70003, CC70A02 and CC70B01 execute as PASS.
  CC70001 remains deferred at the existing generic-child unit-loader gap.
  All 1,319 tests pass; the 129-case baseline is unchanged.
  [Contract](docs/generic-types.md#formal-packages-and-contract-matching).
- [x] **Partial formal-package actual lists and boxes.** Mixed explicit/
  named-box lists and `others => <>`, with selective matching and copied-formal
  visibility/default rules. Dependent types/profiles, package forwarding and
  nested/library replay preserve member identity; matched/private declarations
  stay hidden. Overload identity and implicit `/=` survive copying; invalid
  selectors, duplicates, omissions and association order are diagnosed.
  Four scenarios run whole/per-unit/determinism with four workers; five
  rejection checks and the existing four formal-package ACATS scenarios pass
  compiler/C-runtime ASan/UBSan checks. All 1,333 tests pass and all 129 baseline
  observations remain unchanged; no new partial-specific ACATS PASS is claimed.
  [Contract](docs/generic-types.md#partial-formal-package-actual-lists-and-boxes).
- [x] **Access formal objects.** Named/anonymous object and callback access
  formals support `in` copies, `in out` aliases, defaults, strict designated
  subtype/profile/constant-view matching, null exclusions and existing
  accessibility checks. Anonymous `in out` requires an anonymous actual;
  explicit exclusions retain forwarding checks. Checked `in` initialization
  follows the documented AI12-0287-1 correction. Collection cleanup covers
  normal/exceptional completion and failure of later actuals; nested/library
  replay retains identity and single evaluation. Five scenarios pass
  whole/per-unit/determinism with four workers; the rejection test diagnoses
  invalid actuals/defaults and uninstantiated escaping/forwarding contracts.
  ACATS C3A0020/C3A0025 execute unchanged as PASS; old rejection snapshots
  retired. All 1,349 tests pass; all 129 baseline observations stay unchanged.
  Compiler/C-runtime ASan/UBSan checks pass the five whole/per-unit scenarios,
  rejection test and both ACATS executions (generated Ada accesses remain
  uninstrumented; LeakSanitizer unavailable).
  [Contract](docs/generic-types.md#access-formal-objects).
- [x] **Limited `in` formal objects.** Permitted aggregates and function-call
  actuals/defaults build in place, preserving identity, single evaluation,
  bounds/sliding, class-wide tags/dispatch and local/library lifetime. Existing
  object names cannot be copied; invalid defaults and limited aggregate
  components are rejected without an instance. View-aware checks preserve
  internal copying through nonlimited private completions. Five scenarios
  pass whole/per-unit/determinism with four workers, including controlled
  destruction order and cleanup after constructor, later-actual, aggregate,
  package-elaboration and finalizer failures. Library-level runtime-bound
  array objects reuse 6b storage. All 1,365 tests pass; all 129 baseline
  observations remain unchanged. Existing C650003/CA120012/C730004 stay PASS;
  no new limited-object-specific ACATS PASS is claimed. The five whole/per-unit
  scenarios, rejection test and C650003/CA120012 executions pass compiler/C-runtime
  ASan/UBSan checks (generated Ada accesses remain uninstrumented;
  LeakSanitizer unavailable). Concurrent construction stays diagnosed for P9.
  [Contract](docs/generic-types.md#limited-in-formal-objects).
- [x] **Complete generic contract checking for the features above.** Invalid
  templates/defaults reject without an instance. Copied nested contracts retain
  source name/operator/type/completion resolution and enclosing symbol mappings
  through separate compilation and specification/body continuation. Validated
  `Class` occurrences replay idempotently; repeated `Class` in source still
  rejects. The AARM 12.6(8.i–8.n) nested class-wide abstract-formal exception
  preserves dispatch, defaults and controlling results; illegal direct actuals
  remain rejected. Apply RM 12.3(11,14) and the RM 8.3(26) exception: ordinary
  overloads may collapse in an instance, visible dispatching homographs reject,
  private/body copies retain their source meanings. Explicit private-part null
  checks, generic matching and runtime constraints remain active. Four new
  scenarios pass whole/per-unit/determinism with four workers; legal controls
  and uninstantiated invalid templates are covered. Full suite 1,378/1,378;
  all 129 baseline observations remain unchanged. Compiler/C-runtime ASan/UBSan
  passes the four new and five representative existing scenarios in whole/
  per-unit modes, eight rejection fixtures and unchanged CC51B03/CC70002
  executions (generated Ada accesses uninstrumented; LeakSanitizer unavailable).
  [Contract and completion audit](docs/generic-types.md#contract-checking-and-phase-completion).

**Phase 6g is complete for the documented supported actual types and profiles.**
The existing CC70001 generic-child loader deferral is owned by Phase 8 below;
no complete Ada conformance or concurrent implementation is implied.

For each item, add execution/legality coverage, whole/per-unit and determinism
checks where applicable, and relevant unchanged ACATS scenarios with explicit
PASS/XFAIL/deferred status. Preserve the baseline and document sanitizer limits.
Close each item against its stated language behavior and tests. Phase 6g
completion covers generic semantics over currently supported actual types;
concurrent extensions requiring Phase 9 remain explicitly deferred. Neither the number
of checkboxes nor an unrelated ACATS rejection measures language completeness.

**Generic-related work owned by other phases**

- Phase 8: prefixed-view renaming and generic subprogram actuals (C413005),
  including receiver binding, lifetime and reduced-profile conformance; implicit
  operator callable views/renaming/actuals; generic subprogram renaming-as-body
  completions; the existing CC70001 generic-child loader deferral; full
  staticness/legality audit and B-test grading. These remain
  Phase 8 work rather than implicit additions to 6g.
- Phase 9: concrete synchronized/task/protected interface implementations,
  protected-operation generic actuals, named protected callbacks, task/protected
  discriminants, entries and rendezvous (including deferred CC1207B).
  Concurrent actuals and their execution tests belong here; they are not
  supplied by the completed interface-formal increment and do not turn the
  static synchronized-derived-formal item into a full tasking prerequisite.

### 6h. Pragmas and checks

- [x] Implement the Ada 2005 assertion and check-policy milestone, including
  `Assert`, `Assertion_Policy`, `Suppress`, `Unsuppress` and `No_Return`. Validate
  placement and arguments, enabled/disabled behavior and failure propagation;
  continue diagnosing unsupported pragmas until each is implemented.
  `Ada.Assertions`, lazy messages, ignored-but-checked assertions, source generic
  policies, scoped check permissions and nonreturning cleanup/overrides now work
  in whole/per-unit builds (`pragma*`, `semantics.pragmapolicies`, C651001).
  List/Page/Optimize and noncollecting Controlled are validated. Check omission is optional
  and conservative across package/generic/subunit and C-runtime boundaries.
  Imported No_Return 'Access remains diagnosed pending Phase 7 foreign adapters;
  categorization remains Phase 7, packing/representation and the wider pragma
  audit Phase 8, concurrent policies Phase 9. [Contract](docs/pragmas-and-checks.md).
  The implementation-defined Restrictions set is empty; unsupported restrictions
  diagnose. Task Storage_Size remains with the existing Phase 9 item.
  No_Return renamings of ordinarily returning targets diagnose 'Access until
  the Phase 8 callable-completeness audit supplies guard adapters.
  Validation: 1,408/1,408 tests, all 129 baseline observations unchanged, and
  focused compiler/C-runtime ASan/UBSan checks passed.

Tests: allocation and deallocation through a shared collection from several
tasks, finalization inside an abort-deferred region, defaulted discriminants
with variant changes, a record representation clause with a variant part, a
callback capturing an `aliased` local, an access value escaping its scope,
subunits using enclosing declarations, mutually dependent limited views,
private context visibility, null-exclusion failures, defaulted aggregate
components, noncontrolled extended returns, a formal package, an interface
hierarchy with prefixed dispatch.

## Phase 7 — Predefined library

Inventory the Ada 2005 predefined environment in Phase 8. These are staged
deliverables, not an exhaustive list; distinguish missing Ada 95 packages from
Ada 2005 additions and track annex packages separately.

- [x] `Ada.Strings.Unbounded`, written in Ada on the controlled-type support,
  followed by basic definite `Ada.Containers.Vectors`. Deep ownership, growth,
  bounds, controlled-element lifetime, partial-copy/finalization failure cleanup
  and four-task use pass whole/per-unit/determinism checks (`unboundedstrings`,
  `basicvectors`, `vectorlifetime`, `librarycollectiontasking`,
  `librarycollectionerrors`). Unchanged CXA4010/CXA4011/CXA4031 PASS; CXA4032
  compile-XFAILs at missing Maps.Constants. Mapping callbacks, cursor/callback
  APIs, full container operations and categorization retain their later owners.
  Full suite: 1,425/1,425 checks; focused compiler/C-runtime ASan/UBSan and
  C-runtime TSan pass (generated Ada accesses remain uninstrumented).
  [API, ownership and limits](docs/library-collections.md).
- [x] Complete the Ada 2005 container families: definite and indefinite vectors,
  lists, hashed/ordered maps and sets, plus sorting utilities. Shared controlled
  ownership, indexed vectors, linked lists, hash buckets and AVL trees support
  cursors, callback tampering checks, key projections, set algebra, copy/failure
  cleanup and mutable definite elements (`container*`). Whole/per-unit and
  deterministic execution includes four-task cross-owner deallocation and
  structural AVL invariants. Full suite: 1,465/1,465 checks; focused compiler/
  C-runtime ASan/UBSan and C-runtime TSan pass (generated Ada accesses remain
  uninstrumented). All twelve unchanged 3.1DD Amendment cases
  CXAI001–CXAI006/CXAIA01–CXAIA06 PASS. Streaming and categorization retain
  their later Phase 7 owners; Ada 2012 extensions remain outside the target.
  [API, ownership and limits](docs/library-collections.md).
- [x] `Ada.Directories` and `Ada.Environment_Variables`, with the base
  `Ada.Calendar` prerequisite, UTC microsecond time, checked host operations,
  limited controlled search snapshots and callbacks (`calendarlibrary`,
  `environmentvariables`, `directorieslibrary`, `hostlibrarytasking`,
  `hostlibraryerrors`, `runtime.host_library`). Whole/per-unit/determinism and
  four-task coverage; unchanged CXAH001/CXAH002/C96006A/C96007A PASS.
  Unchanged CXAG001 also PASS after adding REPORT filename support and Text_IO
  string Get. Focused compiler/C-runtime ASan/UBSan and C-runtime TSan pass
  (generated Ada accesses remain uninstrumented).
  Host contracts, forms/patterns, ownership, concurrency and portability
  limits: [host library](docs/host-library.md). Win32 adapters implement native
  environment, drive/UNC paths, handle metadata and reparse-safe deletion;
  MinGW tests are enabled, including `runtime.windows_host_library`.
  Host regression repairs: filesystem-clamped timestamp fixtures, ordinary and
  extended Windows CWD spellings, and driver Windows stack-page probes for large
  generated frames (`driver.windows_stack_probes`, `windowsstackframes`).
  Native MSYS2 and GCC Linux CI checks were confirmed by the user;
  the broader cross-host audit remains Phase 8 work;
  Calendar children retain the later item below.
- [x] Type-aware streaming: root-class file/custom stream dispatch, component
  order, typed bounds/discriminants, Stream_Size, user attributes, renamings and
  generic actuals; controlled Input and tag-aware class-wide dispatch/results.
  Unbounded strings and all twelve container families stream logical values.
  `streammemory`, `streamrecords`, `streamclasswide`, `streamsizes` cover
  whole/per-unit/determinism; `streamcontainers` covers all families and
  indefinite keys/elements; `streamerrors` and `runtime.stream_dispatch` cover
  legality, callback ABI/errors, static links and four concurrent streams.
  Unchanged CDD1001/CDD2B01 PASS; the Range index prerequisite also promotes
  unchanged C3A0013 to PASS. Full suite: 1,511/1,511 checks. Focused compiler/
  C-runtime ASan/UBSan pass; Windows/MinGW C dispatch cross-compiles.
  QBE-generated Ada accesses remain uninstrumented. Native Windows execution
  remains user-verified separately. Format changes, access-discriminant limits,
  upstream blockers and remaining I/O work: [stream contract](docs/streaming.md).
- [x] `Ada.Exceptions`: uppercase expanded names and aliases, wide/wide-wide
  name overloads, logical occurrence streams across partitions, and limited
  occurrence-valued function/allocator results with owned-message cleanup.
  Standard wide scalar/string storage is the prerequisite; Unicode identifiers,
  wide text attributes/I/O and full wide facilities retain their later owner.
  `exceptionlibrary`/`exceptionlibrarytasking` cover whole/per-unit/determinism,
  5,000-character/NUL messages and four tasks. `runtime.exception_library` and
  `runtime.exception_partitions` cover registry/ownership and separate processes.
  Unchanged CB41001/CB41003/CB41004 PASS; CB41002 retains a task-entry blocker.
  Full suite: 1,522/1,522 checks; focused compiler/C-runtime ASan/UBSan and
  C-runtime TSan pass (generated Ada accesses remain uninstrumented).
  C code cross-compiles for
  Windows/MinGW. ABI rebuild, format and remaining wide limits:
  [exception contract](docs/exceptions-library.md).
- [x] `Ada.Strings.Maps.Constants` and mapping-function callbacks: all eleven
  Latin-1 sets and three maps, and Index/Count/Translate overloads for Fixed,
  Bounded and Unbounded. Full-character comparisons, lazy search callbacks,
  null mappings, exception preservation, bounds near Integer'Last and four-task
  use pass whole/per-unit/determinism (`stringmappingcallbacks`,
  `stringmappingtasking`, `stringmappingerrors`). Unchanged CXA4030 and CXA4032
  PASS; the latter is promoted from its missing-Constants compile XFAIL.
  Full suite: 1,530/1,530 checks; focused C-runtime ASan/UBSan and TSan
  pass (generated Ada accesses remain uninstrumented). Categorization and wide
  facilities retain their later owners.
  [API and validation](docs/library-collections.md).
- [x] Narrow `Text_IO` and numeric input: nested numeric/enumeration generics,
  shared growing lexer, Width and string Get/Put, long numeric instances,
  current error/Flush, layout/positions, lookahead/immediate input, String-valued
  Get_Line and bounded/unbounded adapters. All four file families finalize through
  scopes/collections, with borrowed current/standard snapshots and limited-result
  adaptation (`textinputgrammar`, `textiolibrary`, `textstringio`, `iofilelifetime`,
  `iolibrarytasking`, `runtime.text_io_library`; whole/per-unit/determinism and
  four tasks). Unchanged CE3402A/CE3704A/CE3804A/CE3904A PASS. Existing text and
  host fixtures reflect final page termination. Wide I/O retains its later item;
  floating output rounding/locale, cross-host terminals and remaining Stream_IO
  operations retain the Phase 8 environment audit. C-runtime ASan/UBSan and TSan
  and focused compiler ASan/UBSan pass (generated Ada accesses uninstrumented).
  Full suite: 1,550/1,550 checks.
  [Host representation, ownership, API and limits](docs/text-io.md).
  - [x] Explicit-file and current-input String Get previously supplied arbitrary
    bounds/slices, null reads, line/page skipping, EOF/device errors and file
    locking (`textgetstrings`, `runtime.string_get`, `runtime.blocking_io`).
    REPORT.Legal_File_Name enables unchanged CXAG001 to PASS.
- [x] `Ada.Calendar` children: Arithmetic supports full-range day differences
  and checked day operations; Formatting supplies weekday/HMS access, all Split/
  Time_Of profiles and strict date/elapsed images and values; Time_Zones exposes
  the base Calendar's UTC choice. Microsecond Time and Duration's scale remain
  unchanged. Leap-second requests raise Time_Error; reports are zero/False.
  `calendarchildren`, `calendarchecks`, `calendartasking` cover whole/per-unit/
  determinism and four tasks. `runtime.calendar_library` covers every supported
  Gregorian date and minute offset, boundaries and concurrent callers.
  Obsolescent System.Tick enables unchanged C96008A; C96008B also PASS alongside
  C96006A/C96007A. Full suite: 1,562/1,562 checks; focused compiler/C-runtime
  ASan/UBSan and C-runtime TSan pass (generated Ada accesses uninstrumented).
  Host choices, expanded elapsed hours and portability limits:
  [calendar contract](docs/calendar.md).
- [x] Enforcement of package categorization (`Pure`, `Preelaborate`), including
  Ada 2005 rules and `Preelaborable_Initialization`. Checks cover declaration/body
  dependencies, elaboration and binder order, private/protected initialization
  promises, formal actual matching and delayed generic bodies. Ada 2005 access
  types/zero pools and private-view external streaming are checked. Predefined
  categories and applicable initialization promises are in the Ada specs.
  `categorization`, whole/per-unit/determinism, `semantics.categorization`,
  `categorization*errors`, `preelaborable*errors` and unchanged CA21001 cover the
  increment. See [contract](docs/package-categorization.md) for the original
  Ada 2005 generic/limited-view rules and remaining Annex C/E/Phase 8 boundaries.
- [ ] Complete `Wide_Character`, `Wide_String`, `Wide_Wide_Character` and
  `Wide_Wide_String` facilities: basic Standard types/storage/contextual literals
  landed for exception names; required I/O, string/character facilities and
  complete overload resolution remain open, along with Ada 2005 identifier
  character rules and a documented source encoding policy.
- [ ] A foreign ABI and ownership contract for imported controlled results.
  Include guarded foreign callbacks for imported No_Return procedures (their
  'Access is currently diagnosed; direct calls are guarded).

## Phase 8 — Tooling and validation (alongside the phases above)

- [x] Establish Ada 2005 as the language target without claiming full
  conformance; README.md tracks the subset, this roadmap the progress.
- [ ] Maintain an Ada 2005 coverage matrix (skeleton from the Phase 5 prelude)
  by RM clause and predefined unit:
  implemented behavior, restrictions, missing legality/runtime checks, regression
  and ACATS coverage, and owning roadmap item. Include completed features in the
  gap review; classify core requirements, implementation-defined choices,
  extensions and selected Specialized Needs Annex facilities separately.
- [ ] Extend record representation support beyond the Phase 6c byte subset:
  bit-packed and unaligned fields, changed layouts on untagged derived types
  with representation-aware conversions/inherited operations, general freezing
  and nonadjacent/private-part clauses. Track CD4031A (packing XFAIL), CD4051B
  and CD1C04E (changed derived layouts) as applicable upstream cases.
- [ ] Robustness: malformed-source, fuzzing, crash and timeout coverage, and
  sanitizer builds. Bound the run time of the compiler and of test programs.
  From Phase 4, run the tasking tests under ThreadSanitizer. QBE-generated Ada
  memory accesses are not instrumented; find another way to cover them, for
  example a race detector at the run-time boundary or an instrumented IR pass
  outside `qbe/`.
  - [x] Record-component and declaration syntax recovery after missing
    delimiters (`recordrecovery*`, with timeouts).
- [ ] Complete discrete-range syntax using range attributes in array type
  definitions beyond the `array (Object'Range)` form now covered by C3A0013.
  Include subtype range-attribute constraints (C35003A), subtype-mark discrete
  ranges in index constraints/aggregate choices (C36104A/B, C37002A), and
  function-call/qualified-expression array bounds (C43204A).
  Preserve the index type and evaluate bounds once at type elaboration; cover
  explicit dimensions and runtime bounds. Existing `'Range` support in loops
  and other contexts does not cover this declaration syntax. Also support
  range-attribute choices in array aggregates (C3A2003).
- [ ] Complete access-subtype designated-record discriminant constraints
  (C37003A/C37006A); keep the explicit diagnostic until constraint capture,
  null/conversion checks and library/per-unit behavior are implemented.
- [ ] Complete generic-child unit loading for generic parents. Unchanged
  CC70001 is currently rejected by the loader before formal-package execution;
  this is the existing deferral recorded during Phase 6g. Preserve parent and
  child instance identity and dependencies in whole/per-unit loading, then run
  the complete upstream scenario. This belongs to compiler completeness rather
  than reopening the completed formal-package matching increment.
- [ ] Grouped generic formal objects: parse declarations such as `V, V1 : T`,
  retaining each object's mode, defaults, association and instance identity.
  Unchanged CC3601A currently compile-XFAILs at the first comma; rerun the whole
  scenario after the grammar is supported. This is a syntax gap in existing
  ordinary formal objects, separate from 6g's access/limited-object increments.
- [ ] Adopt relevant Ada 2005 ACATS conformance tests (harness from the Phase 5
  prelude) alongside each feature,
  including inherited Ada 95 requirements and cross-feature legality/runtime
  cases. Record unsupported cases and implementation-defined behavior; extend
  coverage as the subset grows without treating a passing selection as full
  conformance. GNAT comparisons are not a project requirement. Correctness
  repairs in the completed subset are owned by
  [6f-prelude](#6f-prelude-acats-gaps-in-supported-features).
  - [x] Completed-feature backfill baseline: 106 unchanged cases with pinned
    provenance, exact observations and triage (`baseline.acats_*`); 37 C PASS,
    37 legal-source rejections, five execution failures, seven support blockers;
    20 B observations remain ungraded, including accepted B36002A.
    Only observed C passes join the regular ACATS label.
    [Baseline and follow-up order](docs/acats-baseline.md).
- [ ] Add B-test grading for expected error locations, allowed diagnostic
  groups, recovery and legal controls. Rejection for an unrelated unsupported
  construct must not count as PASS. Fill REPORT.Ident_Char and TCTouch protocol
  gaps, then applicable target-specific ImpDef support; re-probe blocked cases.
- [ ] Validate supported hosts and targets: ABI widths, installation and
  relocation, library lookup, paths with spaces. Includes validation left open
  by completed items: running the suite on MSYS2 (Phase 2 threading), and
  checking Phase 2's no-slowdown goal on other hosts and targets.
- [ ] Prefixed-view renaming and generic subprogram actuals (ACATS C413005),
  preserving receiver binding, lifetime and reduced-profile conformance.
- [ ] Serialized semantic interfaces, so clients compile without dependency
  sources. `.ali` files hold only build metadata today.
- [ ] Robust artifact publication and concurrent builds: interrupted writes,
  dependency changes or removal, search-path shadowing.
- [ ] Driver: skip scan, bind and link when nothing changed; let `--clean` work
  without a source argument.
- [x] Driver external linkage: accept ordered `.o`/`.a` inputs for executable
  builds, retaining Ada binding/runtime linkage; diagnose inputs in nonlinking
  modes (`driver.external_linkage`).
- [x] Lowering layer decision: see the Phase 5 prelude. Reopen before select
  emission if the existing contracts cannot express its cleanup/cancellation
  paths without duplication, or when an independent backend needs a separate
  representation.

## Phase 9 — Full tasking

Completes Ada 2005 core tasking on top of Phases 4 and 6. Core tasking is part
of the language goal, not an optional annex. Track annex-specific priorities,
scheduling and real-time facilities separately in the Phase 8 coverage matrix.

- [ ] Task/protected aggregates and function results: extend 6e construction
  with caller-master ownership, delayed task activation and failed-result
  cleanup (`semantics.returns` retains explicit rejections).
- [ ] Task entries, `accept` statements and rendezvous, including entry
  families and parameters of every supported mode and type.
- [ ] Selective accept, with guards, `terminate` and `delay` alternatives;
  timed and conditional entry calls; asynchronous `select`.
- [ ] `requeue` and `abort`, on the Phase 6 abort-deferral design (protected
  actions, finalization).
- [ ] Task and protected discriminants, named access-to-protected-subprogram
  types (anonymous protected callbacks landed in 6d), and synchronized, task
  and protected interfaces.
- [ ] Priorities and `Storage_Size`, `Ada.Task_Attributes`, and
  `Ada.Synchronous_Task_Control`.
- [ ] Selected Systems Programming Annex support: Ada 2005 task termination
  notification (`Ada.Task_Termination`). Define separate goals and tests for
  selected Real-Time Systems Annex facilities
  such as execution-time clocks, timers, timing events and Ravenscar restrictions;
  do not infer annex or profile compliance from the minimal tasking subset.

## Completed groundwork

For orientation; extend these rather than restart them. Details are in the README.

- Scalars: checked 32/64-bit integers, modular types up to `2 ** 64`, floating
  point, ordinary and decimal fixed point, local and library runtime bounds.
- Arrays: one- and multi-dimensional, 64-bit descriptors, runtime-bounded
  locals, library objects, components and allocators, aggregate bound
  inference, sliding, overlap-safe slice assignment, scoped storage reclamation.
- Records: grouped field defaults, default and runtime discriminants, mutable
  variants, controlled variant components, representation clauses.
- Calls: context-driven overload resolution, user operators, defaults,
  copy-in/copy-out, caller-owned composite results, fall-through checks.
- Exceptions: occurrences, messages, saved occurrences, re-raise, tracebacks,
  package-body handlers.
- Packages: local packages and generic instances, elaboration order and
  failure reporting.
- Access: shared collections, general and anonymous access, accessibility
  checks, coextensions, named access-to-subprogram types and nested callbacks.
- Renaming: objects, components, subprograms, operators, attributes, exceptions.
- Generics: private, discrete, numeric, fixed and array formals, formal
  subprograms and objects, contract checking.
- Tagged and controlled types (stages 1–4c): inheritance, class-wide values,
  dispatching, `Ada.Tags`, full controlled lifetimes and ownership.
- Tasking: per-task context, masters, entry-free tasks, protected objects and
  entries, delays, `Ada.Real_Time`, atomics, abort-deferral regions.
- Library: `Text_IO` and its generics, sequential, direct and stream I/O,
  numerics, Latin-1 characters, fixed and bounded strings, command line.
- Toolchain: per-unit compilation, manifest binding, incremental caching,
  deterministic output.
