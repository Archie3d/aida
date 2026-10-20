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
   when code generation changes. The full suite stays green.
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
8. **Run validation alongside implementation.** Start Phase 8's coverage
   inventory now and maintain it throughout the roadmap. Add relevant Ada 2005
   ACATS coverage with each feature rather than waiting for the earlier phases
   to finish.
9. **Follow dependencies, not just bullet order.** Keep the phase sequence,
   but resolve local prerequisites before their dependents. In Phase 6,
   establish general limited-record semantics before generalizing limited
   aggregates and build-in-place returns; ordinary noncontrolled extended
   returns can proceed independently. In Phase 7, implement `Ada.Calendar`
   before `Ada.Directories`, whose modification-time operations use
   `Ada.Calendar.Time`, and wide character/string types before wide exception
   names and related library APIs.
10. **Establish tasking's limited-type rules early.** Phase 4 must enforce the
  noncopying, lifetime and finalization rules needed by task/protected objects
  and composites containing them, without waiting for Phase 6's general
  limited-record support. Keep real concurrency in Phase 4 and full core
  tasking in Phase 9; the latter remains a language requirement, not an
  optional annex goal.

## Phase 1 — Stop silent acceptance

Highest priority: constructs that were accepted while their meaning was dropped.
The original items and all follow-ups below are complete, including strict
generic formal type parsing and diagnostics for unsupported constructs.

- [x] **Pragmas.** `Parser::parsePragma` accepts `Import`, plus `Inline`, `Pure`
  and `Preelaborate` as advisory, and rejects every other or unknown pragma in
  context clauses, declarative parts and statements. Covered by `pragmas` and
  the `pragma*errors` tests (syntax, unknown, representation, elaboration,
  checks, tasking).
- [x] **Representation clauses.** Only `'Size` clauses are accepted, and they
  are validated against the representation and range. Other attribute,
  enumeration, record and address clauses are rejected. Covered by
  `sizeclauses`, `representationerrors` and `size{layout,range,value}errors`.
- [x] **Import conventions.** Only C and Ada are accepted. Covered by
  `importconventions`, `importconventionerrors` and
  `importconventionsyntaxerrors`.
- [x] **Subprogram stubs.** `is separate` on a subprogram is rejected. Covered
  by `subprogramstuberrors` and `subprogramcompletions`.
- [x] **Reserved words.** `aliased`, `protected`, `requeue`, `until`,
  `interface` and `synchronized` are reserved, and the constructs they introduce
  have their unsupported forms rejected (including inside generic formal type
  definitions). Phase 4 replaces the original protected-declaration rejection
  with its supported subset. Covered by
  `reservedworderrors`, `reservedconstructerrors`, `reservedgenericerrors` and
  `reservedwordboundaries`.
- [x] **Import without an external name.** Both C and Ada imports require an
  explicit, nonempty external name string. Missing names are diagnosed instead
  of silently dropping the pragma. Covered by `importconventionerrors` and
  `importconventionsyntaxerrors`; explicit-name calls remain covered by
  `importconventions`.
- [x] **Remaining `Import` arguments.** Only three positional arguments are
  accepted. Named associations (including `Convention => ...` and
  `Link_Name => ...`) and extra arguments receive explicit diagnostics.
  Closing delimiters are required and trailing tokens are rejected. Covered
  by `importconventionerrors` and `importconventionsyntaxerrors`.
- [x] **Package body stubs.** `package body P is separate;` is rejected with a
  single clear diagnostic at `separate`, without cascading parse errors.
  Covered by `subprogramstuberrors` in local, nested-package, block and
  uninstantiated-generic contexts.
- [x] **Generic formal type definitions.** Only complete private, limited
  private, discrete, integer, floating-point, ordinary fixed-point and array
  forms are accepted. Unsupported categories and trailing tokens are rejected;
  `range <>` and `digits <>` must end at the box. Covered by
  `genericformalerrors` and `genericformalsyntaxerrors`. The predefined
  deallocator uses supported private-formal syntax with an explicit access
  contract check, covered by `deallocationformalerrors` and the existing
  allocation tests. Phase 6 implements the rejected categories.
- [x] **Clear diagnostics for unsupported syntax.** Abstract subprograms, `goto`
  and labels, `select`, entry-bearing task declarations, `limited with` and `private with`
  receive messages naming the unsupported construct. Recovery consumes task
  definitions and nested select blocks and preserves statements after labels.
  Covered by `unsupportedconstructerrors`, `unsupportedcontexterrors` and
  `unsupportedrecoveryerrors`.

Tests: a rejection test for each unsupported formal type form, and one per
construct in the diagnostics item.

## Phase 2 — Tasking foundations

Prepare the run time and code generator for several threads of control before
more features depend on single-threaded behavior. Nothing here adds Ada syntax;
the existing suite must pass unchanged, with no measurable slowdown.

Current state: the C run time has a bindable per-task context. Each generated
subprogram caches the context returned by traceback entry; exception checks
and handlers load the pending exception at offset zero from that pointer. The
binder fetches its context once as well. C workers can call the tested Ada
subset under separate contexts. Image and stream results are caller-owned;
file and tag tables are locked, and library lifetime storage belongs to the
environment task. The C threading layer provides joinable threads, mutexes,
monotonic condition waits and scalar atomic access. Master entry, activation,
await and exit hooks bracket scoped cleanup; Phase 4 supplies their task behavior.
Empty subprogram masters are elided when the lifetime analysis proves they have
no declarations or owned temporaries.
Finalization chains and storage checkpoints already live in stack frames, and
exceptions propagate by explicit checks rather than unwinding, so neither needs
redesigning. The shared state is in the C run time.

- [x] **Per-task context.** Gather the mutable run-time state into one context
  record: pending exception, message and trace (`adart.c`), the current trace
  frame, and the registries of finalizations and allocations. The main program
  owns the first context; each task will own another. C helpers use a C11
  thread-local binding, with zero-initialized caller-owned contexts and checked
  disposal after masters exit. Registry operations require the owning context.
  Covered by `runtime.task_context` (four concurrent workers, context switching,
  trace/message isolation and cleanup) and `runtime.array_storage` (allocation
  failure and pending-message disposal). Library lifetime storage now belongs
  to the environment context; process-wide command-line state remains separate.
- [x] **Generated access to the context.** QBE's `thread` data cannot serve
  every target: it is rejected for extern symbols on arm64 Linux and entirely on
  Windows amd64 (`qbe/arm64/emit.c`, `qbe/amd64/emit.c`). Have each subprogram
  fetch the context pointer once on entry (a run-time call or a hidden
  parameter), and load the pending exception at a fixed offset from it. Measure
  the cost and pick one ABI before more emitter code reads run-time state.
  Chosen ABI: return the context from the existing traceback-entry call, update
  the local trace frame directly, and pass the context to traceback exit. No
  extra Ada parameter or QBE TLS reference is needed. The hidden-parameter
  entry probe showed no speed advantage on the measured host; generated call
  timing improved over Step 1. See `tests/benchmarks/README.md` for measurements,
  scope and reproduction. Covered by `runtime.generated_contexts`, its IR
  structure checks, `determinism.generatedcontexts`, and updated IR goldens.
  The phase-wide no-slowdown goal still needs broader/other-target validation;
  Step 1's exception-helper-only overhead is not removed by this emitter ABI.
- [x] **Result buffers.** Removed rotating static image, fixed-point text and
  stream-result buffers. Images write into per-expression caller stack storage
  sized from the type, returning explicit lengths; fixed-point images no longer
  allocate. Unconstrained stream input uses the existing owned-array descriptor
  and shared caller-adoption path, releasing partial reads on failure. The 4 KiB
  stream cap is gone; signed 32-bit length/byte-count limits remain. Covered by
  `imageresults`, `streamresults`, their determinism tests and
  `runtime.result_buffers` (four workers, allocation/adoption failure and
  truncated input). The image benchmark is recorded in `tests/benchmarks/README.md`.
- [x] **Shared run-time tables.** Protect the file table and current input/output
  (`adaio.c`), tag registrations (`adatags.c`) and library finalization with a
  run-time lock, or give them a single owner. File operations use per-file mutexes
  across validation and use, with a short table lock and pins preventing reuse; current-file getters return stable handle snapshots
  and file names use caller-owned array results. Tag registration and lookup
  use a separate mutex; descriptors survive until process shutdown after worker
  termination. Library lifetime storage is owned by the environment context;
  foreign-context operations and recursive library shutdown raise Program_Error.
  Process-wide exit status uses relaxed C11 atomic loads/stores, including its
  startup reset; concurrent access and post-join visibility are covered by
  `runtime.task_context`.
  Collections reject allocation from a different context. No generated callback
  runs under an infrastructure table mutex. Covered by `runtime.shared_tables`, `filesnapshots`
  and its determinism test, including concurrent first use and slot reuse.
  `adalock` supplies the private mutex operations needed here, extended by the
  threading layer below. `runtime.blocking_io` verifies unrelated file
  progress during blocked text, stream and fixed reads, including Close/Reset
  waiters; the table lock is never held across blocking I/O or file-lock waits.
- [x] **Threading layer.** `adathread` provides a thin C layer over POSIX threads
  (winpthreads on MSYS2): joinable threads, mutex lifecycle/try-lock, condition
  variables and monotonic nanosecond clocks/deadlines. macOS and winpthreads use
  relative condition waits; other hosts select `CLOCK_MONOTONIC` explicitly. Status
  functions return POSIX errors; callers own thread/context lifetimes.
  Sequentially consistent 8/16/32/64-bit atomic loads/stores operate on naturally
  aligned scalar storage, ready for Phase 4 `Atomic`/`Volatile` lowering.
  Native lock-free support is required, avoiding a new linker dependency.
  Covered by `runtime.threading` (contention, signal/broadcast, timeout,
  thread results and atomic publication), plus `runtime.task_context` and
  `runtime.result_buffers` using the layer. All 355 tests and a focused
  ThreadSanitizer run pass on macOS; the user also verified the suite on Linux.
  MSYS2 execution remains host validation work. No generated code or existing
  lock/unlock path is changed by this step.
- [x] **Masters and activation hooks.** `adamaster` supplies task-free entry,
  activation, await and exit hooks with explicit contexts and caller-owned
  identities. Subprograms and blocks activate after successful declarations;
  the binder activates the library master only after all elaborations succeed.
  Existing managed access-type collections activate after allocator completion
  and await before any member is finalized. Nested scopes await, finalize and
  release storage inside-out on normal completion, return, loop exit and
  exception transfer; handlers retain their own scope until it exits.
  Statically empty subprogram boundaries are elided to preserve scalar leaf
  call cost. Task-valued temporaries must join lifetime analysis in Phase 4.
  Covered by `runtime.masterhooks`, `runtime.masterfailure`,
  `runtime.masterlibraryfailure` and `determinism.masterhooks`, including
  observed finalization order, failed declarations/activation, re-raise,
  missing returns, extended returns, finalizer failure and collection variants.
  The binder IR golden is updated. The existing call/image benchmarks showed
  no material regression on macOS (see `tests/benchmarks/README.md`); broader
  and cross-target performance validation remains open.

Tests: the existing suite; a C test that runs run-time helpers (images,
exceptions, allocation) from several threads at once; IR golden updates for the
new context access.

## Phase 3 — Harden the existing subset

Audits of supported features, in the order below. Write a focused reproduction
before choosing a fix. Follow the language target recorded in Phase 8 and keep
README.md current alongside this phase, recording deliberate subset restrictions and
validating supported cases with focused regression tests. Wider numeric support,
new access forms and other features explicitly deferred to later phases stay
there; this phase does not introduce an anonymous library-block extension.

- [x] **Unit identity and loading.** Establish specification/body identity from
  declarations and define the source naming policy. Diagnose filename/unit-name
  mismatches and conflicting or duplicate definitions; handle repeated references
  to the same source consistently. Cover explicit sources, automatic spec/body
  loading, source argument order, and parent/child dependencies. Represent
  specifications and bodies as separate dependency nodes, preserving each
  part's context clauses. Diagnose illegal dependency cycles without rejecting
  legal dependencies between bodies and already available specifications.
  Code: `adac/UnitLoader.cpp`, `adac/UnitManifest.h`, `adac/UnitManifest.cpp`,
  `adac/main.cpp`.
  - [x] Source-file foundation: declaration-based part identities, preregistered
    explicit paths, canonical-path deduplication, conflicting-source diagnostics,
    automatic spec/body loading, parent dependencies, and separate source-part
    graph ordering with cycle diagnostics. Format 3 manifests preserve that graph.
    Covered by `loading.{cycles,identity,sources,child}`, `ada.load_main`,
    `ada.child_main` and `determinism.load_main`. Explicit custom filenames and
    multi-item source bundles remain supported extensions; library-path lookup
    diagnoses filename/declaration mismatches.
  - [x] Per-library-item identities, context-clause storage and dependency
    ordering within multi-item bundles, including adjacent generic bodies.
    Format 4 manifests and emitted elaborations use declaration identities while
    object/cache names remain file-based. Explicit bundles are preregistered and
    passed through separate compilation. Covered by `loading.bundles` (duplicate,
    cycle, parent, source-order, whole/separate IR and incremental checks),
    `ada.bundle_main`, `ada.bundle_client`, `determinism.bundle_main` and
    `determinism.genericelaborationorder`; existing elaboration-failure tests
    cover stopping within a bundle. Generic template bodies have no standalone
    elaboration effects.
  - [x] Declaration/completion identity audit: null procedures and renamings
    are classified as standalone declarations or completions from the discovered
    declarations, independent of source order. Reject orphan package bodies,
    incompatible package/procedure/function kinds, duplicate completions, bodies
    for already-complete declarations, and non-package parents. Automatic lookup
    validates source-part roles; explicit targets and main selection use source
    identity, including aliases. Covered by `loading.completions`,
    `ada.completion_main` and `determinism.completion_main`, including spec
    addition/removal rebuilds. Generic renaming completions remain explicitly
    unsupported. Parameter-profile conformance and full semantic context
    isolation remain in the existing visibility/conformance audit.
- [x] **Elaboration order and checks.** Build elaboration constraints from the
  per-part dependency graph, including specification-before-body and parent/child
  requirements. Have binding validate manifest order and reject missing,
  duplicate and unknown elaboration entries, as well as unsatisfiable ordering
  constraints. Define and implement elaboration-before-use checks that raise
  `Program_Error` where required, covering direct and indirect calls, local
  bodies and generic instantiations as well as library initializers. Preserve
  failure propagation and cleanup when elaboration stops partway through.
  Apply the same rules to whole-program and separate compilation.
  Code: `adac/main.cpp`, `adac/Binder.cpp`, `adac/UnitManifest.cpp`,
  `adac/sema/`, `adac/emitter/`.
  - [x] Binder graph validation, covered by `binding.elaboration_order`: missing,
    duplicate and unknown entries, unknown dependencies, duplicate part metadata,
    invalid ordering and truncated manifests. Identity-derived specification/body
    and parent/child constraints are enforced even when explicit edges are
    omitted; self-dependencies and cyclic constraints are rejected.
  - [x] Subprogram elaboration-before-use checks at body entry, including
    indirect calls and renaming completions. Library flags are set in source
    order; nested flags belong to each activation and reset on block re-entry.
    `elaborationchecks` covers early/valid calls, repeated activation, generic
    instances and controlled cleanup; `elaborationlibrarycheck` covers partial
    library failure, finalization and skipped main execution. Both have
    whole/separate determinism checks. The controlled-completion fixture now
    initializes its objects in a client package, after the defining bodies.
  - [x] Generic template-body flags and instantiation-site checks for bodies
    available to the token-based expander, including separate library bodies.
    Existing generic executable and determinism suites cover successful
    elaboration; templates still have no instance initialization effects.
  - [x] Forward generic instantiation: publish the instance specification and
    defer body expansion until its template body is available, preserving formal
    bindings, type identity, lexical captures and contract resolution. Generic
    subprograms and packages raise `Program_Error` on early execution, before
    instance initialization; packages with no body remain valid. Covered by
    `forwardelaboration` (required/optional bodies, cleanup and repeated calls),
    `forwardcontracts` (nested deferred instances, distinct actual types and
    declaration/body-site binding), `forwardlibrary` and their determinism tests.
    Missing required bodies get semantic diagnostics (`elaborationgenericerrors`).
    `elaboration.forward_modes` executes whole and separate builds, checks
    template-body changes rebuild clients, and verifies whole-program partial
    elaboration failure cleanup. Elaboration pragmas remain the next item.
- [x] **Elaboration pragmas.** Implement `Elaborate`, `Elaborate_All` and
  `Elaborate_Body`, replacing their Phase 1 rejections. Check placement,
  arguments and named-unit legality. Test each pragma separately: body-before-
  client ordering for `Elaborate`, the required transitive closure for
  `Elaborate_All`, and a required body immediately following its specification
  for `Elaborate_Body`. Diagnose conflicting constraints and carry the pragma
  requirements through scan metadata, binding and incremental rebuilds.
  Context pragmas validate preceding with visibility (including ancestors),
  retain per-item requirements and add body/transitive semantic-dependency
  edges. `Elaborate_Body` accepts library package/generic specifications and
  named pragmas following library subprogram declarations or instantiations;
  instance bodies already elaborate with their specifications. Library generic
  pragmas do not apply to instances. Format 5 manifests preserve expanded edges
  and explicit immediate-body constraints, checked again by the binder.
  Impossible constraints use deterministic dependency-cycle diagnostics.
  Covered by `ada.ep_main`, `determinism.ep_main`, `elaboration.pragmas` and
  updated `pragmaelaborationerrors`: whole/separate execution and IR, child
  units, generics, bodyless units, bundles, placement/name/argument errors,
  required bodies, direct/transitive/adjacency cycles, malformed metadata and
  incremental changes to pragmas and transitive bodies. Specification-only
  compilation defers unavailable dependency-body ordering to the full scan and
  binder; source dependencies continue to use the existing digest cache.
- [x] **Visibility and conformance.** Audit visibility, `use` clauses,
  homographs, duplicate declarations, overload ambiguity, private views,
  parent/child visibility and context-clause isolation between compilation
  units. Check specification/body conformance, including parameter modes,
  names, defaults, subtype constraints and return profiles. Distinguish valid
  overloads from mismatched or duplicate completions. Extend existing local
  missing-body checks to required package and subprogram completions across
  units, reporting them before linking while allowing declarations that do not
  require bodies. Code:
  `adac/sema/SemaNames.cpp`, `adac/sema/SemaDecl.cpp`, `adac/Scope.cpp`.
  - [x] Reject type and subtype declarations that conflict with existing local
    names, preserving incomplete/private type completions and nested-scope
    shadowing. Covered by `duplicatetypeerrors` and `typedeclarations`.
  - [x] Reject named numbers and exceptions (including exception renamings)
    that conflict with existing local declarations, including case-insensitive
    duplicates and repeated names in grouped declarations. Preserve nested
    shadowing, hiding of use-visible names and renamed exception identity.
    Covered by `duplicatenumbererrors`, `duplicateexceptionerrors`,
    `numberexceptiondeclarations` and its determinism test.
  - [x] Subprogram homographs, duplicate profiles/completions and formal names;
    package/generic declarations and enumeration literal conflicts. Completions
    check names, modes, statically matching parameter/result subtypes and
    structurally conforming defaults with resolved declaration identity.
    Omitted body defaults remain valid; generic contract replay preserves
    overloads that acquire identical actual types. Covered by
    `duplicateprofileerrors`, `profileconformanceerrors`, `defaultbindingerrors`,
    `profiledefaults`, their executable/determinism coverage and existing
    generic, renaming and overload suites.
  - [x] Direct visibility takes precedence over use visibility across enclosing
    scopes; conflicting use-visible names are diagnosed. Private declarations,
    private use clauses and package-body locals do not leak to clients or the
    visible parts of public children. Package shadowing and child subprogram
    namespaces preserve legal lookup and completion. Covered by
    `visibilityerrors`, `visibilityconformance`, `profiledefaults`, existing
    primitive/private-view tests and `semantics.visibility_conformance`.
  - [x] Each library item has its own context, including items in source bundles.
    Specifications pass their context to bodies and descendants; body imports
    stay local. Library children require a mentioning with clause, `System`
    requires a with clause, and `Standard` supports expanded names. Generic
    contract/body environments preserve body-site captures. Updated older
    fixtures to state their dependencies and qualify ambiguous exceptions.
  - [x] Missing required local/library subprogram and generic bodies are
    diagnosed before linking, including unresolved package members and
    mismatched overload completions. Driver scanning now performs semantic
    validation before publishing manifests or reusing objects; `adac -c` still
    permits specification-only compilation. Covered by
    `requiredcompletionerrors`, `semantics.visibility_conformance` (whole and
    separate builds, child privacy, source-order/bundle isolation, dependency
    edits, stale artifacts, valid bodyless/import/null declarations and IR
    determinism), plus the full regression suite.
    Audit baseline: Ada 95/2005 RM 6.3.1, 7.2, 8.3, 8.4 and 10.1.2. Existing
    extensions remain: explicit source bundles/custom filenames and optional
    library package bodies without `Elaborate_Body`. Wider primitive-operation
    legality remains in its separate Phase 3 item.
- [x] **Blocks executed during package-body elaboration.** Executable parts
  and handlers use a per-unit elaboration frame for block locals, captured
  objects, nested subprograms and local packages. Package declarations retain
  their library lifetime. Blocks reuse scoped storage, masters, finalization
  and access collections on normal completion, loop exit and exceptions;
  declaration failures bypass the block's handlers. Covered by
  `elaborationblocks` (including separate-compilation determinism),
  `elaborationblockfailure` (cleanup before failed library startup) and
  `elaborationblockerrors` (return legality and block visibility). Runtime-bound
  block arrays are supported; library-level runtime bounds remain in Phase 6.
- [x] **Aggregates.** Static and dynamic array targets share checked shape,
  choice, index-subtype and sliding logic; `others` fills only unassociated
  components. Ordinary record assignments build a temporary before copying,
  preserving self-references and the destination on component failure. Record
  associations enforce ordering, component names, coverage, duplicates and
  a common type for grouped choices. Defaults in ancestor-type extension
  aggregates and grouped expressions are evaluated per component.
  Covered by `aggregateaudit`, `aggregateauditchecks`, `aggregateauditerrors`
  and `aggregatecleanup`, with determinism tests, alongside existing inferred,
  runtime, multidimensional and controlled aggregate regressions. Includes null
  arrays, partial initialization cleanup and component subtype checks.
  Ada 2005 box defaults (`<>`) remain unsupported with an explicit diagnostic
  (`aggregateboxerrors`); variant/discriminant restrictions remain as documented
  in their roadmap items. Audit baseline: Ada 95/2005 RM 4.3.1 and 4.3.3.
- [x] **Array conversions.** Ordinary and controlled value conversions check
  every dimension, slide to constrained targets and check unconstrained index
  subtypes. Null ranges skip subtype checks while enumeration/modular bounds
  must still be representable. Writable converted actuals evaluate the operand
  once, check entry constraints and convert back only after normal return;
  same-family arrays with private or tagged components retain element identity.
  Ada and imported calls share this handling. Value conversions, constants and
  function results cannot be written through; cross-type views with private or
  tagged components are rejected.
  Covered by `arrayconversions`, `arrayconversionchecks`, `arrayconversionerrors`
  and determinism tests, alongside `controlledconversions`. Includes 32/64-bit
  integer and enumeration indexes, null/multidimensional shapes, runtime
  constraints, out/in out calls, copy-back failure and exception propagation.
  The cross-type restriction remains exact component subtype object identity:
  distinct subtype declarations with equal constraints are still rejected.
  No relaxation was made. Limited controlled array conversions and bounds
  beyond the existing 32-bit descriptors remain unsupported. Audit baseline:
  Ada 95/2005 RM 4.6 and 6.4.1.
- [x] **Derived types.** Audit derivation legality, inherited constraints and
  primitive operations, type identity and explicit conversion rules. Cover
  accepted and rejected conversions and runtime range, discriminant and tag
  checks where applicable to the supported subset.
  - [x] Related untagged enumeration, record and access value conversions,
    including siblings and multiple derivation levels; retain distinct implicit
    type identity and hide ancestry introduced by private completions. Preserve
    inherited scalar base ranges and constrained record discriminants. Check
    explicit scalar/discriminant conversions even when nested or given static
    operands. Derived access types share their parent's collection and lifetime.
    Inherited calls through private scalar views initialize `in` temporaries
    without copying them back. Reject incomplete parents, repeated discriminant
    constraints, untyped conversion operands and writable non-array untagged
    conversions; new discriminant parts and derivation before a private view's
    completion receive explicit unsupported diagnostics.
    Covered by `derivedtypes`, `derivedconstraints`, `derivedaccess`,
    `derivedlibrary`, their whole/separate determinism tests and
    `derivedtypeerrors`; existing `primitive*`, `tagged*`, `classwide*` and
    `arrayconversion*` suites retain inheritance, tag-check and array coverage.
    Audit baseline: Ada 2005 RM 3.4, 3.4.1, 4.6 and 7.3.1. These are focused
    RM-based regressions, not imported ACATS cases.
  - [x] Derived Boolean scalar conditions, logical operators and short-circuit
    forms retain their type identity; overload resolution accepts any visible
    Boolean type in conditions. Derived Character literals and byte-character
    arrays retain element identity through concatenation; string literals check
    constrained component ranges. Private completions
    do not expose these categories outside their package. Inherited record
    profiles retain the parent's discriminant constraints independently of the
    derived first subtype, including unconstrained results through multiple
    derivations. Constrained record formals check discriminants before entering
    the callee for all parameter modes. Covered by `derivedcategories`,
    `derivedcategoryerrors`, `derivedprofiles` and determinism tests, including
    cross-library calls, failed checks without body execution or object mutation,
    and constrained destinations receiving unconstrained inherited results.
    Additional baseline: Ada 2005 RM 4.5.1, 5.3 and 6.4.1.
    Writable non-array untagged conversions, new derived discriminant parts,
    derivation before private-view completion, and string literals/concatenation
    with character components wider than a byte remain diagnosed restrictions.
    The wider primitive visibility/overriding and variant-record audits below
    remain separate items; this completes derivation within the supported subset.
- [x] **Attributes.** `'Pos`, `'Val`, `'Succ`, `'Pred`, `'First`, `'Last` and
  `'Range`: base versus subtype rules, valid type/object prefixes, operand
  types, arity and result width. Include scalar endpoints, null ranges,
  multidimensional array dimension arguments, and static versus runtime
  failures within the existing numeric model.
  - [x] Validate visible scalar subtype marks, array object/type prefixes,
    argument types and counts, and static array dimensions. Discrete position
    results retain universal-integer context and width; value and adjacent-value
    attributes use the scalar base range, including derived and dynamic subtypes.
    Diagnose static operands outside the prefix base range before folding.
    Check endpoints before narrowing, with `Constraint_Error` for failed value
    conversions and successor/predecessor operations. Fixed-point adjacency uses
    `Small`; floating-point adjacency uses the next representable machine value.
    Array bounds retain their index width, and a lowered array `'Range` evaluates
    its prefix once, including function results and implicit access dereferences.
    Covered by `attributeaudit`, `attributebounds`, their determinism tests, and
    `attributeauditerrors`; existing attribute and runtime-bound tests remain.
    Audit baseline: Ada 2005 RM 3.5, 3.5.5 and 3.6.2. These are focused RM-based
    regressions, not imported ACATS cases. The existing 32/64-bit numeric model,
    32-bit array descriptor bounds and deferred static-arithmetic work remain
    unchanged; broader representation and streaming attributes remain separate
    roadmap items.
- [x] **Primitive operations.** Wider visibility and legality audit for tagged
  and untagged primitives: declaration placement, inheritance, hiding,
  overriding indicators, controlling profiles and dispatch through supported
  class-wide views. Include private views and separate compilation.
  - Added primitive-indicator checks, distinguished inner operator hiding from
    overriding, and enforced subtype conformance for tagged overrides, including
    operations inherited at a private completion. Controlling defaults must be
    tag indeterminate; new private controlling-result functions on visible
    tagged types are rejected. Multiple controlling types are checked on both
    new and overriding operations.
  - Tagged objects and subprogram bodies now freeze primitive declarations,
    alongside the existing derivation check. Older controlled-type fixtures now
    declare their overriding profiles before the first body. Covered by
    `primitiveauditerrors`, `primitiveplacementerrors`, and existing
    `primitiveerrors`/`classwideerrors` legality tests.
  - `primitiveaudit` and its whole/separate-compilation determinism test cover
    private tagged completion, private overriding, class-wide dispatch with an
    inherited controlling default, and local hiding. Existing `primitives`,
    `primitivevisibility`, `primitivechecks`, `primitiveseparate`, derived-profile
    and class-wide tests retain untagged inheritance and runtime-check coverage.
    Baseline: Ada 2005 RM 3.2.3, 3.4, 3.9.2, 3.9.3, 8.3, 8.3.1 and 13.14;
    these are focused RM-based regressions, not imported ACATS cases.
    Tagged discriminants, private extensions, anonymous controlling access
    profiles and general freezing beyond these declaration triggers remain
    outside this audit's coverage; wider type support and the Phase 8 coverage
    inventory remain open.
- [x] **Variant records.** Confirm that nested variant parts get correct layout,
  initialization and selection checks at every level. Cover assignment,
  copying, aggregates and discriminant mismatches for supported constrained
  objects, retaining active-component equality regressions. Mutable
  discriminants and controlled variant components remain in Phase 6.
  Nested variant parts now retain their enclosing alternatives throughout
  parsing, layout, aggregate resolution, initialization, selection and equality.
  Only active defaults run; runtime selections check every enclosing selector.
  Assignment through an unconstrained record formal checks the actual target's
  discriminants before copying, preserving it on failure. Covered by
  `nestedvariants`, its determinism test, and `nestedvariantserrors`: three-level
  nesting, sibling nested parts, null/others alternatives, mixed 32/64-bit field
  alignment, positional/named aggregates, function results, arrays and record
  components, copying, equality and discriminant mismatches at every level.
  Existing `variants`, `varianterrors` and aggregate/comparison regressions remain
  in place. Baseline: Ada 2005 RM 3.7, 3.8.1, 4.1.3, 4.3.1, 4.5.2 and 5.2;
  focused RM-based regressions, not imported ACATS cases. Explicit record
  constraints still require static discriminants, and unconstrained standalone
  objects, mutable discriminants and controlled variant components remain
  unsupported with diagnostics.
- [x] **Feature interactions.** Add focused combinations of the audited features
  with existing overload resolution, named/defaulted arguments, generic
  instantiation, renaming, controlled ownership and exception handling. Check
  copy-in/copy-out behavior, owned results and partial initialization cleanup;
  preserve the Phase 2 master/context boundaries on normal and exceptional
  exits. Reuse existing regression coverage and add cases for uncovered
  interactions rather than duplicating entire feature suites.
  - `interactioncalls` combines overloaded renamings of generic instances,
    renamed parameter names/defaults, constrained array conversions and selected
    slices. Checks single evaluation, sliding, normal copy-back, no copy-back
    after exceptions, entry-shape rejection and scalar copy-back range failure.
    `interactionerrors` rejects constant and function-result converted actuals
    passed by name through a renamed generic instance.
  - `interactionownership` combines private generic actuals, formal function
    wrappers, overloaded/defaulted renamings, controlled aggregate results and
    array conversions. Resource counts verify early-return cleanup, partial
    aggregate initialization failure, preserved exception messages, failed
    adjustment in a forwarding wrapper and successful recovery over repeated
    activations. Synthetic formal-subprogram wrappers now record owned-result
    finalization requirements, preserving their master and temporary ownership
    chains instead of emitting a missing owner argument.
  - Both executable tests have whole/separate IR determinism coverage;
    `semantics.interaction_modes` additionally executes whole-program builds.
    Reuse `runtime.masterhooks`, `runtime.masterfailure`,
    `runtime.masterlibraryfailure`, `runtime.task_context` and
    `runtime.generated_contexts` for master ordering and context isolation;
    `aggregatecleanup`, `controlledconversions`, `forwardcontracts`,
    `primitiveaudit` and `nestedvariants` retain the existing cross-feature
    coverage. No runtime ABI or supported-language boundary changes.
    Baseline: Ada 2005 RM 6.4.1, 7.6.1, 8.5.4 and 12.6; focused RM-based
    regressions, not imported ACATS cases. The existing array-view, generic
    formal and controlled-variant restrictions remain in their owning items.

Completion criteria for every item: record the supported behavior and any
remaining restrictions; add positive executable tests, legality rejection
tests and runtime-failure tests as applicable. Cover the cases named above,
including valid dependency patterns alongside cycle failures, malformed
elaboration manifests, each pragma's ordering and legality, calls before body
elaboration, and valid completions alongside each conformance mismatch.
Exercise whole-program and separate compilation for cross-unit behavior,
including rebuilds after changes to dependencies or elaboration pragmas.
Validate supported behavior against the Ada 2005 Reference Manual,
accounting for documented implementation-defined behavior. Add determinism tests
whenever a change affects generated code, update README.md with changed behavior,
and keep the full suite green. Mark an item complete only when its audit and
applicable regression coverage are complete.

## Phase 4 — Minimal tasking
A working subset, close to the Ravenscar profile, that makes tasking a tested
part of the compiler early, without claiming Ravenscar compliance. Later phases
must keep it working; Phase 9 completes core tasking.

- [x] **Tasks.** Task types and single task objects without entries; activation
  at the end of the declarative part or library elaboration; masters awaiting
  task objects as record components and allocated tasks.
  - Task bodies reuse lexical frames and scoped cleanup. Objects are created
    without starting threads; the complete group checks body elaboration before
    starting any member. The activator waits for all declarations to finish;
    activation failures raise `Tasking_Error`, while an exception escaping a
    task's statements completes only that task. Failed enclosing declarations
    discard tasks that never reached activation. Library activation precedes main.
  - Runtime masters keep empty boundaries allocation-free, own task contexts
    and join dependents before finalization. Local package specifications/bodies
    have separate activation groups, including generic instances. Arrays and
    ordinary records initialize each task component in place. Allocated tasks
    belong to the access type's collection and its original master, even when
    allocated from a shorter-lived subprogram; failed allocations cancel their
    pending tasks before storage release. Nested allocators retain separate
    activation groups, and task slots retain pointer alignment in mixed records.
  - Tasks and composites containing them cannot be copied or assigned; renaming
    and by-reference parameters preserve identity. Task discriminants, entries,
    tagged/variant task composites, derivation, task-containing aggregates,
    explicit initialization/defaults, conversions, streaming and function results
    receive diagnostics. Access-type collections retain the Phase 2 ownership
    restriction: allocation/deallocation from another task raises `Program_Error`.
    General shared collections remain a Phase 9 follow-up.
  - Covered by `tasks`, `taskallocations`, `tasklibrarymain`,
    `taskactivationfailure`, their whole/separate IR determinism checks,
    `taskerrors` and `tasklegalityerrors`. `runtime.tasking` executes generated
    concurrency in both compilation modes: bounded rendezvous, activation
    handshakes, local/generic package groups, normal/exceptional finalization,
    allocated components and exception-message preservation.
    `runtime.tasking_lifecycle` covers repeated contexts, mixed successful/failed
    groups, body-readiness checks and collection ownership of nested allocations.
    Baseline: Ada 2005 RM 3.11, 7.5, 7.6.1 and 9.1–9.3; focused RM-based
    regressions, not imported ACATS cases. ThreadSanitizer passes for the runtime
    lifecycle test and both generated concurrency modes with instrumented C
    runtime/helpers; QBE-generated Ada memory accesses are not instrumented.
    The C lifecycle test also passes AddressSanitizer and UndefinedBehaviorSanitizer.
- [x] **Protected objects.** Protected types and single protected objects with
  procedures and functions, under one lock per object.
  - Private data has a hidden receiver, with constant current-instance views in
    functions, including nested helpers and aliases. External calls evaluate the
    receiver and actuals before acquiring the object's mutex, then release it
    before exception transfer and copy-back. Functions conservatively use the
    same exclusive mutex as procedures. Internal calls reuse the current action;
    current-instance expanded names and private/local operations retain that rule.
  - Locks join the existing finalization chains for local, library, allocated,
    array and ordinary-record objects. Task masters await users before cleanup;
    partial construction and failed allocation release initialized locks only.
    Overloads, named/defaulted arguments, composite results, generic protected
    units and whole/separate compilation retain the existing call ABI around
    the hidden receiver. Private views continue to hide protected operations.
  - Protected objects and containing composites cannot be copied, assigned,
    converted, streamed or returned as values in this subset.
    Discriminants, interfaces, operator functions, operation renaming, protected
    subprogram access and protected operation generic actuals remain diagnosed.
    Private components
    containing controlled, task or protected parts, tagged/variant composites,
    and layouts/value operations preceding a protected private completion are
    also rejected. Access collections retain their Phase 2 owner restriction.
    External self-calls and task creation during an action raise `Program_Error`;
    this is not general `Detect_Blocking` support.
  - Covered by `protectedobjects`, `protectedconcurrency`, `protectedlibrary`,
    `protectedgenerics` and `protectedcleanup`, executable whole/separate modes
    and IR determinism; `protectederrors`, `protectedlegalityerrors`,
    `protectedcompletionerrors`, `protectedgenericerrors` and
    `protectedsyntaxerrors` cover legality and unsupported recovery. `runtime.protected_lifecycle` checks contention,
    independent-object progress, action stacks, re-entry, exception preservation
    and disposal. Baseline: Ada 2005 RM 7.5, 7.6.1, 9.4 and 9.5.1; focused
    RM-based regressions, not imported ACATS cases. ThreadSanitizer and
    AddressSanitizer/UndefinedBehaviorSanitizer pass for the C runtime tests and
    the five generated cases in both compilation modes with instrumented C
    runtime/helpers; QBE-generated Ada memory accesses remain uninstrumented.
- [x] **Protected entries.** Single entries with Boolean barriers and queued
  calls on protected types and single protected objects.
  - Barriers run under the object's execution lock. Closed calls retain their
    actuals and lexical environment while waiting; each entry's queue is FIFO.
    Procedure and entry completion service eligible queues until none is open,
    before returning to callers. Protected functions do not service queues.
    Entry `'Count` is available within the current protected instance, including
    barriers; calls are checked before and after queuing.
  - Entry bodies use the existing parameter ABI, including named/defaulted
    actuals, scalar copy-back, unconstrained array views, nested helpers and
    overloads. Copy-back follows queue servicing and release of the lock.
    Barrier exceptions raise `Program_Error` in queued entry callers; entry-body
    exceptions propagate to their caller. Servicing preserves a pending
    exception's identity, message and trace in the completing operation.
  - Calls to entries from a protected action raise `Program_Error`; statically
    internal entry calls are diagnosed. Entry families, timed/conditional/select
    calls, requeue and task entries remain diagnosed and outside this item.
  - `protectedentries`, `entryproducer`, `entrylibrary`, `entrygenerics` and
    `entryexceptions` cover producer/consumer blocking, FIFO, queue draining,
    barrier failure, entry-body exceptions, copy-back failure, allocation,
    single objects, lexical captures and overloads in whole/separate compilation
    and IR determinism. `entryerrors` and `protectedsyntaxerrors` cover legality
    and unsupported syntax. `runtime.entry_lifecycle` exercises multiple C
    producers/consumers, barrier failures across queues and pending messages.
    Baseline: Ada 2005 RM 9.5.2, 9.5.3 and 9.9; focused RM-based regressions,
    not imported ACATS cases. ThreadSanitizer and AddressSanitizer/
    UndefinedBehaviorSanitizer pass for the C entry/protected runtime tests and
    the five generated cases in both compilation modes with instrumented C
    runtime; QBE-generated Ada memory accesses remain uninstrumented.
- [x] **Time.** Standard `Duration`, relative `delay`, `delay until` for
  `Ada.Real_Time.Time`, and the Ada 2005 `Ada.Real_Time` API.
  - `Duration` uses signed 64-bit counts with `Small = Delta = 2**(-30)`
    seconds, range `-2**33 .. 2**33 - 2**(-30)` seconds, and the existing
    ordinary fixed-point arithmetic, conversions, attributes and generic I/O.
    Relative delays require this predefined type even when its name is hidden.
  - `Time` and `Time_Span` use signed 64-bit nanoseconds: their first/last
    values are `-2**63` and `2**63-1` nanoseconds, `Time_Unit = 1.0E-9`,
    `Time_Span_Zero = 0`, and `Time_Span_Unit = 1` nanosecond.
    `Seconds_Count` is a signed 64-bit integer. Arithmetic, comparisons, unit
    constructors, duration conversions and `Split`/`Time_Of` are checked;
    conversions round nearest, ties away from zero. Negative times split with
    a nonnegative fractional second; composition uses wide intermediates.
  - Clock and both delay forms use the host `CLOCK_MONOTONIC` epoch, without
    synchronization to an external time reference. `Tick` is the host-reported
    monotonic resolution, at least one nanosecond. Positive relative intervals
    round upward to nanoseconds. Interrupted sleeps retry the original deadline;
    expired, zero and negative delays yield without blocking. Expressions are
    evaluated once. Delays during protected actions raise `Program_Error`.
  - `timevalues`, `timedelays` and `timelibrary` cover arithmetic, endpoints,
    rounding, overflow, private views, out-parameter copy-back, generic Duration,
    environment-task elaboration, library tasks, periodic local tasks and queue
    interactions in whole/separate compilation and IR determinism. `timeerrors`
    checks delay types and hidden representations. `runtime.time_lifecycle`
    checks numeric boundaries, real waits, signal interruption on POSIX and
    protected-action rejection. ThreadSanitizer and AddressSanitizer/
    UndefinedBehaviorSanitizer pass for the time/threading C tests and all three
    generated cases in both modes with instrumented C runtime; QBE-generated
    Ada memory accesses remain uninstrumented.
  - Baseline: Ada 2005 RM 9.6 and D.8; focused RM-based regressions, not
    imported ACATS cases. Host scheduling, suspension and clock-source behavior
    govern lateness and clock metrics; hard real-time latency, jitter and drift
    guarantees are not provided. `Ada.Calendar`, select delays, cancellation
    and abort remain in their later roadmap items.
- [ ] **Shared variables.** `pragma Atomic` and `pragma Volatile` through the
  Phase 2 threading layer.
- [ ] **Task identity.** `Ada.Task_Identification`, `'Terminated` and
  `'Callable`.

Tests: producer/consumer through a protected entry, tasks inside a subprogram
and a block, a library-level task, task failure, finalization ordered after
task termination, periodic tasks with `delay until`.

## Phase 5 — Numeric and representation foundations

These unblock several Phase 6 features, especially wider arrays.

- [ ] **Static arithmetic.** Separate universal/static arithmetic from machine
  arithmetic. Diagnose invalid static expressions without host overflow or
  premature narrowing; accept the most negative 64-bit literal.
- [ ] **Real edge cases.** Negative exponents and zero divisors. Document the
  supported floating-point model and its checks.
- [ ] **Floating-point `'Value`.** `Sema` rejects real prefixes, although the run
  time already has `__ada_real_value` for `Float_IO`.
- [ ] **Array descriptors.** Widen bounds and lengths beyond 32 bits, add
  per-dimension strides, and audit size, offset and allocation-overflow
  arithmetic. Lengths are limited to `Integer'Last` today.
- [ ] **Concatenation.** General one-dimensional `&` with element/array
  combinations; it is currently specialized to character arrays.
- [ ] **Modular types.** Unsigned 64-bit storage and `mod 2 ** 64`, modular
  generic formals, the Ada 2005 `'Mod` attribute and `Ada.Text_IO.Modular_IO`.
- [ ] **Ordinary fixed point.** Wider scale choices, explicit `Small` clauses,
  dynamic subtype bounds, remaining attributes and streaming. Apply the Ada 2005
  fixed-by-fixed multiplication and division rules.
- [ ] **Decimal fixed point.** `delta ... digits` types with decimal scaling,
  rounding and checks.
- [ ] **Representation clauses.** Implement alignment, enumeration
  representation and record representation clauses, one at a time, replacing
  their Phase 1 rejections. Also let `'Size` change subtype storage and
  composite layouts.
- [ ] **Checked arithmetic speed.** Inline the overflow checks that now call C
  helpers, keeping the same failure behavior.

Tests: literal endpoints, `2.0 ** (-3)`, huge array sizes, integer array
concatenation, signed small representations, invalid size clauses.

## Phase 6 — Complete the core language

Complete the remaining Ada 95 foundations and Ada 2005 additions together.
Grouped by dependency. Within a group, work top to bottom. Reuse the existing
controlled-result construction, cleanup and dispatch mechanisms.

### Dynamic data

- [ ] Runtime scalar constraints in anonymous subtype indications, library
  declarations, real subtypes, `'Width` and streaming.
- [ ] Library-level arrays and types with runtime bounds, including runtime-bound
  generic array objects (`runtimearraylibraryerrors.ads` lists the cases).
- [ ] Runtime-constrained array components and allocators, with `'Size` and
  streaming.
- [ ] Ada 2005 box defaults (`<>`) in record and array aggregates: component
  default initialization, legality, per-component evaluation and partial-failure
  cleanup. Replace the current `aggregateboxerrors` rejection coverage.

### Discriminated records

- [ ] Default discriminants, constrained versus unconstrained objects, and
  assignments that legally change discriminants.
- [ ] Runtime discriminant constraints and components whose bounds depend on
  discriminants, such as `String (1 .. Length)`.
- [ ] Controlled components in variant records.

### Access types

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

### Program structure

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

### Object orientation

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

### Generics

Code: `Parser::parseGenericDeclaration`, `Sema::bindGenericFormals`,
`Sema::analyzeGenericInstantiation`.

- [ ] Parse and enforce all Ada 2005 formal type categories. Replace the strict
  Phase 1 rejections with derived, access, modular, decimal and interface formals
  as those types become available.
- [ ] Abstract formal subprograms, null procedure defaults, attribute and
  enumeration-literal actuals, and matching of runtime array constraints.
- [ ] Formal packages and matching of their generic contracts, including Ada
  2005 partial actual parameter lists and box associations.
- [ ] Access formal objects, limited `in` objects built in place, and
  library-level runtime-bound array formals.
- [ ] Extend contract checking to each new formal category, and remove the
  remaining legality rechecks done at instantiation.

Tests: defaulted discriminants with variant changes, a callback capturing an
`aliased` local, an access value escaping its scope, subunits using enclosing
declarations, mutually dependent limited views, private context visibility,
null-exclusion failures, defaulted aggregate components, noncontrolled extended
returns, a formal package, an interface hierarchy with prefixed dispatch.

### Pragmas and checks

- [ ] Implement the remaining Ada 2005 core pragmas and check policies, including
  `Assert`, `Assertion_Policy`, `Suppress`, `Unsuppress` and `No_Return`. Validate
  placement and arguments, enabled/disabled behavior and failure propagation;
  continue diagnosing unsupported pragmas until each is implemented.

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
  arithmetic, formatting and time-zone child packages.
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
- [ ] Maintain an Ada 2005 coverage matrix by RM clause and predefined unit:
  implemented behavior, restrictions, missing legality/runtime checks, regression
  and ACATS coverage, and owning roadmap item. Include completed features in the
  gap review; classify core requirements, implementation-defined choices,
  extensions and selected Specialized Needs Annex facilities separately.
- [ ] Robustness: malformed-source, fuzzing, crash and timeout coverage, and
  sanitizer builds. Bound the run time of the compiler and of test programs.
  From Phase 4, run the tasking tests under ThreadSanitizer.
  - [x] Record-component syntax recovery preserves record/variant boundaries;
    declaration recovery preserves the next declaration's introducer after a
    missing delimiter. Covered by `recordrecoveryaccess`,
    `recordrecoverymissingsemicolon` and `recordrecoveryerrors`, with timeouts.
- [ ] Adopt relevant Ada 2005 ACATS conformance tests alongside each feature,
  including inherited Ada 95 requirements and cross-feature legality/runtime
  cases. Record unsupported cases and implementation-defined behavior; extend
  coverage as the subset grows without treating a passing selection as full
  conformance. GNAT comparisons are not a project requirement.
- [ ] Validate supported hosts and targets: ABI widths, installation and
  relocation, library lookup, paths with spaces.
- [ ] Serialized semantic interfaces, so clients compile without dependency
  sources. `.ali` files hold only build metadata today.
- [ ] Robust artifact publication and concurrent builds: interrupted writes,
  dependency changes or removal, search-path shadowing.
- [ ] Driver: skip scan, bind and link when nothing changed; let `--clean` work
  without a source argument.
- [ ] Consider a lowering layer between semantic analysis and QBE emission once
  descriptors, cleanup and dispatch make direct AST emission too cumbersome.

## Phase 9 — Full tasking

Completes Ada 2005 core tasking on top of Phases 4 and 6. Core tasking is part
of the language goal, not an optional annex. Track annex-specific priorities,
scheduling and real-time facilities separately in the Phase 8 coverage matrix.

- [ ] Shared access-type collections across tasks: allocation/deallocation,
  registry ownership and finalization after all dependent tasks terminate.
- [ ] Task entries, `accept` statements and rendezvous, including entry
  families and parameters of every supported mode and type.
- [ ] Selective accept, with guards, `terminate` and `delay` alternatives;
  timed and conditional entry calls; asynchronous `select`.
- [ ] `requeue`, `abort`, and abort-deferred regions (protected actions,
  finalization).
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

- Scalars: checked 32/64-bit integers, modular types up to `2 ** 32`, floating
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
