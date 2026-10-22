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

Complete. Constructs that were accepted while their meaning was dropped now get
diagnostics; later phases replace each rejection as they implement it.

- [x] **Pragmas.** `Import` is implemented; `Inline`, `Pure` and `Preelaborate`
  are advisory; every other pragma is rejected (`pragmas`, `pragma*errors`).
- [x] **Representation clauses.** Validated `'Size` clauses (Phase 5 adds Small) are accepted
  (`sizeclauses`, `representationerrors`, `size*errors`).
- [x] **Imports.** C and Ada conventions only, with exactly three positional
  arguments and a nonempty external name (`importconvention*`).
- [x] **Stubs.** Subprogram and package body stubs (`is separate`) get a single
  diagnostic without cascading errors (`subprogramstuberrors`).
- [x] **Reserved words.** `aliased`, `protected`, `requeue`, `until`,
  `interface` and `synchronized` are reserved; their unsupported constructs are
  rejected, including in generic formals (`reserved*`).
- [x] **Generic formal types.** Private, limited private, discrete, integer,
  floating, ordinary/decimal fixed and array formals only; other categories and trailing
  tokens are rejected (`genericformal*errors`, `deallocationformalerrors`).
- [x] **Unsupported syntax.** Abstract subprograms, `goto`/labels, `select`,
  task entries, `limited with` and `private with` get named diagnostics with
  recovery (`unsupported*errors`).

## Phase 2 — Tasking foundations

Complete. Run-time and code-generation groundwork for several threads, with no
new Ada syntax. Finalization chains and storage checkpoints live in stack frames
and exceptions propagate by explicit checks, so neither needed redesign.
Measurements are in `tests/benchmarks/README.md`; open cross-host validation
moved to Phase 8.

- [x] **Per-task context.** Pending exception, message, trace frame and the
  finalization/allocation registries live in one context record, bound through
  C11 thread-local storage. The environment task owns library lifetime storage;
  stack registry operations require the owning context (`runtime.task_context`,
  `runtime.array_storage`). Phase 6a subsequently shares collection registries.
- [x] **Generated access to the context.** QBE `thread` data is unusable for
  arm64 Linux externs and Windows amd64, so the traceback-entry call returns the
  context; generated code loads the pending exception at offset zero and passes
  the context to traceback exit. No hidden parameter or TLS reference
  (`runtime.generated_contexts`, `determinism.generatedcontexts`).
- [x] **Result buffers.** Images, fixed-point text and stream results use
  caller-owned storage instead of rotating static buffers; signed 32-bit length
  limits remain (`imageresults`, `streamresults`, `runtime.result_buffers`).
- [x] **Shared run-time tables.** Per-file mutexes with a short table lock and
  pins, a locked tag registry and an atomic exit status. No generated callback
  runs under a table mutex, and blocking I/O never holds the table lock
  (`runtime.shared_tables`, `runtime.blocking_io`, `filesnapshots`).
- [x] **Threading layer.** `adathread` wraps POSIX threads (winpthreads on
  MSYS2): threads, mutexes, monotonic condition waits and sequentially
  consistent 8–64-bit atomics requiring native lock-free support
  (`runtime.threading`).
- [x] **Masters and activation hooks.** `adamaster` brackets subprograms,
  blocks, collections and the library master with entry, activation, await and
  exit hooks; scopes await, finalize and release inside-out on every exit path.
  Statically empty boundaries are elided (`runtime.master*`,
  `determinism.masterhooks`).

## Phase 3 — Harden the existing subset

Complete. Audits of supported features against the Ada 95/2005 RM, each with
focused regressions (not ACATS), whole/separate compilation and determinism
tests where code generation changed. Remaining restrictions are listed per item
and in README.md.

- [x] **Unit identity and loading.** Declaration-based identities per library
  item, separate spec/body dependency nodes with their own context clauses,
  canonical-path deduplication, automatic spec/body lookup with
  filename/declaration checks, completion classification and cycle diagnostics
  (`loading.*`, `ada.{load,child,bundle,completion}_main`). Source bundles and
  custom filenames remain extensions; generic renaming completions are
  unsupported.
- [x] **Elaboration order and checks.** The binder validates manifest entries,
  order and implied spec/body and parent/child constraints. Subprogram and
  generic bodies have elaboration flags raising `Program_Error` on early calls
  or instantiations; forward generic instances defer body expansion until the
  template body is available (`binding.elaboration_order`, `elaboration*`,
  `forward*`).
- [x] **Elaboration pragmas.** `Elaborate`, `Elaborate_All` and
  `Elaborate_Body` with placement, argument and visibility checks; requirements
  flow through format 5 manifests, binding and incremental rebuilds, and
  impossible constraints report cycles (`elaboration.pragmas`,
  `pragmaelaborationerrors`, `ada.ep_main`).
- [x] **Visibility and conformance.** Duplicate and homograph declarations,
  direct-over-use visibility, private-part and body-local isolation, per-item
  contexts, full spec/body conformance (names, modes, subtypes, defaults) and
  missing required bodies diagnosed before linking (`duplicate*errors`,
  `profile*`, `visibility*`, `requiredcompletionerrors`,
  `semantics.visibility_conformance`). Library package bodies without
  `Elaborate_Body` remain optional.
- [x] **Blocks during package-body elaboration.** A per-unit elaboration frame
  holds block locals, captures and nested units, with the usual scoped cleanup
  (`elaborationblock*`). Library-level runtime bounds remain in Phase 6.
- [x] **Aggregates.** Shared checked shape, choice and sliding logic for
  arrays; record aggregates go through a temporary, with ordering and coverage
  checks and per-component defaults (`aggregateaudit*`, `aggregatecleanup`).
  Box defaults are rejected (`aggregateboxerrors`).
- [x] **Array conversions.** Per-dimension checks and sliding; writable
  converted actuals evaluate once and copy back only on normal return
  (`arrayconversion*`, `controlledconversions`). Cross-type views require
  identical component subtypes; limited controlled array conversions are
  unsupported.
- [x] **Derived types.** Untagged conversions, inherited constraints and
  profiles, derived Boolean/character categories, and derived access types
  sharing their parent's collection (`derived*`). Writable non-array untagged
  conversions, new discriminant parts, derivation before private completion and
  character components wider than a byte remain diagnosed.
- [x] **Attributes.** `'Pos`, `'Val`, `'Succ`, `'Pred`, `'First`, `'Last` and
  `'Range` with prefix, arity, base-range and width rules and single prefix
  evaluation (`attributeaudit*`, `attributebounds`).
- [x] **Primitive operations.** Overriding indicators, hiding versus
  overriding, subtype conformance, controlling defaults and results, and
  freezing by objects and bodies (`primitiveaudit*`, `primitiveplacementerrors`).
  Tagged discriminants, private extensions, anonymous controlling access
  profiles and general freezing remain open.
- [x] **Variant records.** Nested variants keep their enclosing alternatives
  through layout, aggregates, initialization, selection and equality; assignment
  through unconstrained formals checks discriminants first (`nestedvariants*`).
  Explicit constraints must be static; mutable discriminants and controlled
  variant components remain in Phase 6.
- [x] **Feature interactions.** Overloads, renamed generic instances, defaults,
  array conversions, controlled ownership and exceptions in combination,
  checking copy-back and cleanup on every exit (`interaction*`,
  `semantics.interaction_modes`).

## Phase 4 — Minimal tasking

Complete. A tested subset close to the Ravenscar profile, without claiming
compliance; Phase 9 completes core tasking. Every item runs in whole and
separate compilation with IR determinism. ThreadSanitizer, AddressSanitizer
and UBSan pass with an instrumented C run time; QBE-generated accesses are
uninstrumented (see Phase 8). RM baseline: 3.11, 7.5, 7.6.1, 9.1–9.10, C.6,
C.7.1 and D.8.

- [x] **Tasks.** Entry-free task types and single tasks, grouped activation
  after declarations or library elaboration, masters joining dependents before
  finalization, and task components and allocated tasks owned by their
  collection's master (`tasks`, `taskallocations`, `tasklibrarymain`,
  `taskactivationfailure`, `task*errors`, `runtime.tasking*`). Tasks and
  containing composites cannot be copied; discriminants, entries, tagged or
  variant task composites, aggregates and function results are diagnosed.
- [x] **Protected objects.** Procedures and functions under one exclusive lock
  per object, a hidden receiver, and locks on the finalization chains
  (`protected*`, `runtime.protected_lifecycle`). Not copyable; discriminants,
  interfaces, operation renaming/access and generic actuals are diagnosed.
  External self-calls and task creation in an action raise `Program_Error`.
- [x] **Protected entries.** Single entries with Boolean barriers, FIFO queues
  serviced on procedure/entry completion, `'Count`, and copy-back after release
  (`protectedentries`, `entry*`, `runtime.entry_lifecycle`). Barrier exceptions
  raise `Program_Error` in queued callers. Families, select forms and requeue
  are diagnosed.
- [x] **Time.** `Duration` (64-bit, `Small = 2**(-30)`), relative `delay`,
  `delay until` and `Ada.Real_Time` on 64-bit nanoseconds and the host
  monotonic clock (`time*`, `runtime.time_lifecycle`). No hard real-time
  guarantees; delays in protected actions raise `Program_Error`.
- [x] **Shared variables.** `Atomic` and `Volatile` scalar objects and types
  through sequentially consistent run-time loads and stores, with snapshots and
  checked copy-back for actuals (`shared*`, `semantics.shared_ir`). Shared
  composite parts and component pragmas are diagnosed; aligned RAM only.
- [x] **Task identity.** `Ada.Task_Identification`, `'Identity`, `'Callable`,
  `'Terminated` and entry `'Caller`. Entry bodies run in the caller's task, so
  `Current_Task` there returns the caller (`taskidentity*`). `Abort_Task`
  waits for Phase 9.

Task entries, select/requeue, abort, shared collections (Phase 6) and
Ravenscar compliance remain for later phases.

## Phase 5 — Numeric and representation foundations

Complete for the documented subsets below. These foundations unblock several
Phase 6 features, especially wider arrays. Remaining language and conformance
gaps retain their later-phase owners in the coverage matrix.

### Prelude

- [x] **Validation baseline.** RM-clause and predefined-unit skeleton in
  `docs/ada2005-coverage.md` records completed Phases 1–4, restrictions, regression
  evidence and roadmap owners. Offline CTest ACATS selection: C52005A/C53007A
  pass; C59002A is an explicit compile XFAIL for goto/labels, with pinned
  diagnostics and XPASS detection (`acats.*`). Sources retain upstream rights
  notices and provenance. The local REPORT adapter supports only these C-tests;
  B-tests, full support protocol and Ada 2005-specific additions remain open
  (`tests/acats/README.md`). Phase 8 maintains and extends both baselines.
- [x] **Lowering decision.** Retain direct QBE emission with explicit layout,
  call/result and scope-exit contracts rather than a separate lowering IR.
  [Decision and migration gates](docs/lowering-decision.md) weigh strides,
  discriminant-dependent components, build-in-place results, cleanup, dispatch
  and `select`. Phase 8 records closure; implementation stays with the named
  Phase 5/6/9 features.

### Numeric and representation work

- [x] **Static arithmetic.** Universal integer named numbers and checked exact
  scalar folding are separate from machine arithmetic; signed 64-bit literal
  endpoints, maximal-expression base checks, static check failures and skipped
  short-circuit operands are covered (`staticarithmetic*`, `integerliteral*`,
  `determinism.staticarithmetic`, ACATS C49022A). Final real rounding is binary
  nearest/even. The bounded implementation retains 127-bit integer/rational
  magnitudes and 64-bit scaled fixed intermediates, with explicit diagnostics;
  arbitrary precision and the full RM staticness audit remain unsupported/open
  ([limits and evidence](docs/static-arithmetic.md); Phase 8 coverage matrix).
- [x] **Real edge cases.** Checked binary32/64 arithmetic and real powers with
  Integer exponents, including negatives and `Integer'First`; zero divisors,
  nonfinite results and narrowing overflow raise `Constraint_Error`. Static
  zero-to-negative powers are diagnosed; exact powers use squaring without the
  old exponent-128 restriction. `Machine_Overflows`/`Machine_Rounds` are True
  (`realedges`, `realtasking`, `realedgeerrors`, `runtime.real_arithmetic`,
  whole/separate/determinism checks, ACATS C45523A). C runtime ASan/UBSan and
  ThreadSanitizer checks pass. The [floating model](docs/floating-point.md)
  records default-environment requirements, underflow behavior and remaining
  static-capacity, model-attribute and Annex G restrictions.
- [x] **Floating-point `'Value`.** Scalar base results for Float, Long_Float,
  derived types and constrained prefixes; decimal/based Ada syntax with signs,
  underscores and exponents. Exact rational parsing rounds once to binary32/64,
  independent of locale; malformed text and overflow raise `Constraint_Error`,
  underflow is gradual. No fixed input buffer (`realvalue*`, `runtime.real_value`,
  whole/separate/determinism and task-context sanitizer checks).
  [Contract and remaining I/O boundary](docs/floating-point.md#floating-value).
- [x] **Array descriptors.** Shared local/parameter/result layouts and view
  accessors precede ABI v2: signed 64-bit bounds, lengths and byte strides;
  checked extents, offsets, byte/bit sizes and allocation arithmetic. Wide
  indexing, slices, sliding, captures, aggregates, results and streams migrate
  together; incompatible caches and serialized bounds require rebuilding.
  (`arraydescriptor*`, `arrayframeerrors`, `runtime.array_descriptors`, whole/separate/determinism,
  C runtime/task-context sanitizer checks). [Contract and limits](docs/array-descriptors.md).
- [x] **Concatenation.** General nonlimited one-dimensional `&` in all four
  element/array combinations, including components wider than a byte, private
  and controlled components. Preserve RM bounds and binary-node checks;
  checked sizes, component constraints and controlled temporary cleanup share
  the descriptor and ownership machinery (`concatenation`, `concattasking`,
  `concaterrors`, `runtime.concatenation`, mode/determinism and sanitizer checks,
  ACATS C45347A). [Contract and limits](docs/array-descriptors.md#concatenation).
- [x] **Modular types.** Unsigned 64-bit storage through `mod 2 ** 64`, modular
  generic formals, Ada 2005 `'Mod` and `Ada.Text_IO.Modular_IO`; unsigned
  arithmetic, conversions, attributes, case choices and task-local failures
  (`modular*`, whole/separate/determinism, `runtime.modular`, sanitizer checks,
  ACATS C354002). Array index subtypes and variant choices retain signed 64-bit
  bounds; compact unsigned 32-bit storage remains unsupported.
  [Contract and limits](docs/modular-types.md).
- [x] **Ordinary fixed point.** Binary scales `2.0**(-62)` through `2.0**62`,
  adjacent explicit Small clauses, named local dynamic bounds, scalar attributes
  and checked fixed streams. Ada 2005 fixed-by-fixed numeric result contexts,
  explicit Standard operations and primitive-selection rules (`fixedwide*`,
  `fixedoperator*`, `fixedstreams`, `fixedtasking`, `runtime.fixed_arithmetic`,
  whole/separate/determinism, sanitizer checks and ACATS C35A08B).
  [Contract and remaining restrictions](docs/fixed-point.md).
- [x] **Decimal fixed point.** Exact decimal scales `10.0**(-18)` through
  `10.0**18`, 1–18 digits, digits constraints, truncating arithmetic and Round,
  mixed numeric conversions, decimal formals and Decimal_IO (`decimal*`,
  `runtime.decimal`, whole/separate/determinism and sanitizer checks;
  unchanged ACATS CXF2002/CXF2003). [Contract and limits](docs/decimal-fixed-point.md).
- [x] **Representation clauses.** Implement alignment and enumeration
  representation clauses, one at a time, replacing their Phase 1 rejections.
  Also let `'Size` change subtype storage and composite layouts. Record
  representation clauses follow discriminated records in Phase 6.
  - [x] Alignment first-subtype clauses and subtype/object attributes: 0 and
    powers of two through 8, adjacent representation items, inherited alignment,
    record padding and aligned composite storage (`alignmentclauses`,
    `alignmenterrors`, whole/separate/determinism and concurrent tasks).
    Natural alignment cannot be weakened; scalar/array stride padding,
    object clauses, tagged/task/protected types and general freezing remain
    unsupported. [Contract](docs/representation-clauses.md).
  - [x] Enumeration representation clauses: adjacent positional/named static
    mappings with distinct increasing signed 32-bit codes, 8/16/32-bit storage,
    inherited and overridden derived mappings, checked imports/streams and
    positional attributes/indexing/loops (`enumrepresentation`, `enumstreams`,
    `enumtasking`, `enumrepresentationerrors`, `runtime.enumeration`,
    whole/separate/determinism and unchanged ACATS CD3014A/CD3014D).
    Boolean/Character-derived mappings, wider codes, general freezing and
    clauses after a private-view subtype remain unsupported. [Contract](docs/representation-clauses.md).
  - [x] Size-changing subtype and composite layouts: static scalar subtype
    storage with base-width arithmetic; byte-aligned trailing record/array
    storage, component strides, adapted actuals/results and checked streams
    (`sizestorage`, `sizestreams`, `sizetasking`, `sizestorageerrors`,
    `semantics.type_storage`, whole/separate/determinism and ASan/UBSan).
    Named-subtype Size clauses are an AIDA extension to Ada 2005. Bit packing,
    dynamic layouts and nonconfirming tagged/controlled/limited/task/protected
    sizes remain diagnosed; ACATS CD2A22A is an explicit packing XFAIL, using
    unchanged official Length_Check support. [Contract](docs/representation-clauses.md).
- [x] **Checked arithmetic speed.** Signed 32/64-bit add, subtract, multiply,
  divide, rem/mod, negation, abs and powers now check inline, with the same
  Constraint_Error and cleanup paths. Common 64-bit products avoid the general
  unsigned bound division. Whole/separate differential tests cover 88,000
  boundary/generated cases per mode; concurrent protected cleanup, deterministic
  IL and helper-call exclusion are checked. ACATS C45504A/C45611A pass.
  Named operator calls now resolve Left/Right (unary Right), reordered/mixed
  associations and user-defined formal names without changing overload selection;
  malformed calls get operator/association diagnostics (`namedoperators`,
  `namedoperatorerrors`, whole/separate/determinism). ACATS C45503A now passes.
  [Contract](docs/checked-arithmetic.md) and
  [measurements](tests/benchmarks/README.md#checked-integer-arithmetic).

Tests: an ACATS harness smoke run, literal endpoints, `2.0 ** (-3)`, huge
array sizes, integer array concatenation, signed small representations,
invalid size clauses.

## Phase 6 — Complete the core language

Complete the remaining Ada 95 foundations and Ada 2005 additions together.
Grouped by dependency into milestones 6a–6h, taken in order; within a group,
work top to bottom. Prefixed calls and the 6h pragmas have no prerequisites
here and may be pulled forward. Reuse the existing controlled-result
construction, cleanup and dispatch mechanisms.

### 6a. Tasking prerequisites

- [x] Abort-deferral foundation: task-owned nesting counter, balanced protected
  actions (including entry servicing/queue failures) and existing chain/view
  finalization paths. Explicit call effects and return insertion points replace
  textual call/return recognition; shared master exits retain cleanup order.
  Four-task runtime/Ada, whole/separate and determinism tests (`abortdeferral`,
  `runtime.abort_deferral`, protected/entry lifecycle); 83 baseline units retain
  identical IR. [Design and Phase 9 delivery gates](docs/abort-deferral.md).
  Abort delivery, blocking cancellation, deferred Initialize/whole controlled
  assignments and master waits remain Phase 9; no abort ACATS pass is claimed.
- [x] Shared access-type collections: environment-context indexes synchronize
  allocation/deallocation and heap finalization records across tasks; plain
  scalar/composite allocators now use the same lifetime/cleanup path. Derived
  access types retain their ancestor's collection. Collection tasks belong to
  the access type's original master, whose waits resample concurrent dependents
  before local finalization. Callbacks and joins run outside service locks
  (`sharedcollections`, `sharedtaskcollections`, `runtime.shared_collections`,
  whole/separate/determinism, ASan/UBSan and runtime ThreadSanitizer).
  ACATS C761002 is an unchanged goto/label XFAIL pending 6e, not an executable
  pass. Foreign stack chains, custom pools and general access extensions remain
  unsupported. [Ownership and limits](docs/shared-collections.md).

### 6b. Dynamic data

- [ ] Runtime scalar constraints in anonymous subtype indications, library
  declarations, real subtypes, `'Width` and streaming.
- [ ] Library-level arrays and types with runtime bounds, including runtime-bound
  generic array objects (`runtimearraylibraryerrors.ads` lists the cases).
- [ ] Runtime-constrained array components and allocators, with `'Size` and
  streaming.
- [ ] Ada 2005 box defaults (`<>`) in record and array aggregates: component
  default initialization, legality, per-component evaluation and partial-failure
  cleanup. Replace the current `aggregateboxerrors` rejection coverage.

### 6c. Discriminated records

- [ ] Default discriminants, constrained versus unconstrained objects, and
  assignments that legally change discriminants.
- [ ] Runtime discriminant constraints and components whose bounds depend on
  discriminants, such as `String (1 .. Length)`.
- [ ] Controlled components in variant records.
- [ ] Record representation clauses (moved from Phase 5), including variant
  parts and discriminant-dependent components, replacing the Phase 1 rejection.

### 6d. Access types

- [ ] General access types: `aliased` objects, `access all`, `'Access` of
  objects, access-to-constant, and accessibility checks.
  `access all` and `access constant` currently receive explicit unsupported-type
  diagnostics, covered by `generalaccesserrors`.
- [ ] Anonymous access types in all Ada 2005 contexts, including access-to-constant
  and anonymous access-to-subprogram parameters with nested callbacks. Enforce
  accessibility on calls, conversions, assignments and returned access values.
- [ ] Null exclusions (`not null`): profile conformance, initialization,
  conversions, generic matching and required runtime checks.
- [ ] `'Access` of imported subprograms and of inherited tagged operations.

### 6e. Program structure

- [ ] Limited and private `with` clauses, including `limited private with`:
  incomplete and tagged incomplete views, visibility and completion rules.
  Distinguish limited dependencies from ordinary semantic and elaboration edges
  so legal mutual dependencies survive loading, binding and incremental builds.
- [ ] Separate subunits for subprogram and package bodies: loading, and
  analysis in the parent's scope. Replaces the Phase 1 diagnostic.
- [ ] Package and generic renaming (object, subprogram and exception renaming
  exist).
- [ ] Labels and `goto`, with legality checks for transfers between scopes.
- [ ] Extended return statements for results that are not controlled types,
  general limited aggregates and build-in-place results. Extend the existing
  limited controlled-result support to all required Ada 2005 cases, preserving
  object identity, accessibility, master lifetimes and failure cleanup.

### 6f. Object orientation

- [ ] Abstract subprograms and their full legality rules.
- [ ] Prefixed calls (`Object.Operation (...)`), including inherited primitives,
  class-wide dispatch, access prefixes and overload resolution. Complete Ada
  2005 overriding-indicator and null-procedure legality beyond the existing subset.
- [ ] Discriminated tagged types, private extensions, extensions of private
  views, general limited record definitions, and tagged size clauses.
- [ ] Interfaces, after abstract subprograms: multiple progenitors, inherited
  operations, overriding, interface conversions and class-wide dispatch. Extend
  the single-parent tag/dispatch model with a representation that also supports
  synchronized, task and protected interfaces (Phase 9).

### 6g. Generics

Code: `Parser::parseGenericDeclaration`, `Sema::bindGenericFormals`,
`Sema::analyzeGenericInstantiation`.

- [ ] Parse and enforce all Ada 2005 formal type categories. Replace the strict
  Phase 1 rejections with derived, access and interface formals (modular/decimal added in Phase 5)
  as those types become available.
- [ ] Abstract formal subprograms, null procedure defaults, attribute and
  enumeration-literal actuals, and matching of runtime array constraints.
- [ ] Formal packages and matching of their generic contracts, including Ada
  2005 partial actual parameter lists and box associations.
- [ ] Access formal objects, limited `in` objects built in place, and
  library-level runtime-bound array formals.
- [ ] Extend contract checking to each new formal category, and remove the
  remaining legality rechecks done at instantiation.

### 6h. Pragmas and checks

- [ ] Implement the remaining Ada 2005 core pragmas and check policies, including
  `Assert`, `Assertion_Policy`, `Suppress`, `Unsuppress` and `No_Return`. Validate
  placement and arguments, enabled/disabled behavior and failure propagation;
  continue diagnosing unsupported pragmas until each is implemented.

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

- [ ] `Ada.Strings.Unbounded`, written in Ada on the controlled-type support.
  Then a basic `Ada.Containers.Vectors`. Cover copying, growth, element
  lifetime, bounds and exception cleanup.
- [ ] Complete the Ada 2005 container families: definite and indefinite vectors,
  lists, hashed/ordered maps and sets, plus sorting utilities. Test cursors,
  tampering checks, copying, finalization and generic contract interactions.
- [ ] `Ada.Directories` and `Ada.Environment_Variables`, with documented host
  behavior and error handling.
- [ ] Type-aware streaming: components, bounds and discriminants, user-defined
  stream attributes and a real stream abstraction. Streams mostly copy object
  representation today. Then add controlled and tag-aware class-wide streaming.
- [ ] `Ada.Exceptions`: expanded names (now uppercase defining identifiers),
  wide names, occurrence streaming and occurrence-valued function results.
- [ ] `Ada.Strings.Maps.Constants` and mapping-function callbacks, which
  access-to-subprogram support now permits.
- [ ] Remaining `Text_IO` and numeric input grammar cases.
- [ ] `Ada.Calendar`, on the Phase 4 `Duration`, followed by Ada 2005 calendar
  arithmetic, formatting and time-zone child packages. The base package has no
  Phase 5–6 prerequisites and may be pulled forward.
- [ ] Enforcement of package categorization (`Pure`, `Preelaborate`), including
  Ada 2005 rules and `Preelaborable_Initialization`.
- [ ] `Wide_Character`, `Wide_String`, `Wide_Wide_Character` and `Wide_Wide_String`,
  their required I/O, string and character facilities, and Ada 2005 identifier
  character rules with a documented source encoding policy.
- [ ] A foreign ABI and ownership contract for imported controlled results.

## Phase 8 — Tooling and validation (alongside the phases above)

- [x] Establish Ada 2005 as the language target, distinguishing the currently
  implemented subset from the goal without claiming full conformance. Track
  supported behavior and restrictions in README.md and progress in this roadmap.
- [ ] Maintain an Ada 2005 coverage matrix (skeleton from the Phase 5 prelude)
  by RM clause and predefined unit:
  implemented behavior, restrictions, missing legality/runtime checks, regression
  and ACATS coverage, and owning roadmap item. Include completed features in the
  gap review; classify core requirements, implementation-defined choices,
  extensions and selected Specialized Needs Annex facilities separately.
- [ ] Robustness: malformed-source, fuzzing, crash and timeout coverage, and
  sanitizer builds. Bound the run time of the compiler and of test programs.
  From Phase 4, run the tasking tests under ThreadSanitizer. QBE-generated Ada
  memory accesses are not instrumented; find another way to cover them, for
  example a race detector at the run-time boundary or an instrumented IR pass
  outside `qbe/`.
  - [x] Record-component syntax recovery preserves record/variant boundaries;
    declaration recovery preserves the next declaration's introducer after a
    missing delimiter. Covered by `recordrecoveryaccess`,
    `recordrecoverymissingsemicolon` and `recordrecoveryerrors`, with timeouts.
- [ ] Adopt relevant Ada 2005 ACATS conformance tests (harness from the Phase 5
  prelude) alongside each feature,
  including inherited Ada 95 requirements and cross-feature legality/runtime
  cases. Record unsupported cases and implementation-defined behavior; extend
  coverage as the subset grows without treating a passing selection as full
  conformance. GNAT comparisons are not a project requirement.
- [ ] Validate supported hosts and targets: ABI widths, installation and
  relocation, library lookup, paths with spaces. Includes validation left open
  by completed items: running the suite on MSYS2 (Phase 2 threading), and
  checking Phase 2's no-slowdown goal on other hosts and targets.
- [ ] Serialized semantic interfaces, so clients compile without dependency
  sources. `.ali` files hold only build metadata today.
- [ ] Robust artifact publication and concurrent builds: interrupted writes,
  dependency changes or removal, search-path shadowing.
- [ ] Driver: skip scan, bind and link when nothing changed; let `--clean` work
  without a source argument.
- [x] Lowering layer decision: keep direct semantic-AST-to-QBE emission;
  no separate lowering IR before the descriptor rework.
  [Rationale, scope and migration order](docs/lowering-decision.md) assign
  shared layout/view contracts to Phase 5 descriptors, explicit call/exit
  handling to Phase 6a, component/result/dispatch plans to Phase 6b–6f, and
  select operation plans to Phase 9. These are pending feature prerequisites,
  not completed refactors. Reopen before select emission if those contracts
  cannot express its cleanup/cancellation paths without duplication, or when
  an independent backend/pass requires a separate representation.

## Phase 9 — Full tasking

Completes Ada 2005 core tasking on top of Phases 4 and 6. Core tasking is part
of the language goal, not an optional annex. Track annex-specific priorities,
scheduling and real-time facilities separately in the Phase 8 coverage matrix.

- [ ] Task entries, `accept` statements and rendezvous, including entry
  families and parameters of every supported mode and type.
- [ ] Selective accept, with guards, `terminate` and `delay` alternatives;
  timed and conditional entry calls; asynchronous `select`.
- [ ] `requeue` and `abort`, on the Phase 6 abort-deferral design (protected
  actions, finalization).
- [ ] Task and protected discriminants, access-to-protected-subprogram types,
  and synchronized, task and protected interfaces.
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
  point, ordinary fixed point, and local runtime subtype bounds.
- Arrays: one- and multi-dimensional, runtime-bounded locals, aggregate bound
  inference, sliding, identity rules, overlap-safe slice assignment, scoped
  storage reclamation.
- Records: grouped field defaults, discriminant-constrained variants, and
  active-component equality.
- Calls: context-driven overload resolution, user operators, defaults,
  copy-in/copy-out, caller-owned composite results, fall-through checks.
- Exceptions: occurrences, messages, saved occurrences, re-raise, tracebacks,
  package-body handlers.
- Packages: local packages and generic instances, elaboration order and
  failure reporting.
- Access: object allocators with collections, and named access-to-subprogram
  types with nested callbacks.
- Renaming: objects, components, subprograms, operators, attributes, exceptions.
- Generics: private, discrete, numeric, fixed and array formals, formal
  subprograms and objects, contract checking.
- Tagged and controlled types (stages 1–4c): inheritance, class-wide values,
  dispatching, `Ada.Tags`, full controlled lifetimes and ownership.
- Library: `Text_IO` and its generics, sequential, direct and stream I/O,
  numerics, Latin-1 characters, fixed and bounded strings, command line.
- Toolchain: per-unit compilation, manifest binding, incremental caching,
  deterministic output.
