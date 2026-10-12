# Compiler improvement strategy

AIDA compiles a substantial, regression-tested Ada subset (see the README's
"Supported language" section). This roadmap sets out how to grow it into a
dependable compiler. It is ordered by risk: first remove ways a program can be
silently miscompiled, then lay the foundations tasking needs, harden what
already works, deliver a first tasking subset, and only then widen the language
and library.

Tasking is not left until last. It touches the run time, masters, elaboration,
exception state and the generated code's access to global state, and every
feature built on single-threaded assumptions makes it harder to retrofit.
The foundations and a minimal tasking slice therefore come before the large
language extensions, and the full tasking model follows them.

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
  are rejected (including inside generic formal type definitions). Covered by
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
  and labels, `select`, task declarations, `limited with` and `private with`
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
await and exit hooks now bracket scoped cleanup; they remain task-free until
Phase 4. Empty subprogram masters are elided when the lifetime analysis proves
they have no declarations or owned temporaries.
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
  runs under a runtime mutex. Covered by `runtime.shared_tables`, `filesnapshots`
  and its determinism test, including concurrent first use and slot reuse.
  `adalock` supplies the private mutex operations needed here, extended by the
  threading layer below. `runtime.blocking_io` verifies unrelated file
  progress during blocked text, stream and fixed reads, including Close/Reset
  waiters; the table lock is never held across blocking I/O or file-lock waits.
- [x] **Threading layer.** `adathread` provides a thin C layer over POSIX threads
  (winpthreads on MSYS2): joinable threads, mutex lifecycle/try-lock, condition
  variables and monotonic nanosecond clocks/deadlines. macOS uses relative
  condition waits; other hosts select `CLOCK_MONOTONIC` explicitly. Status
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
- [ ] **Elaboration order and checks.** Build elaboration constraints from the
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
    invalid ordering and truncated manifests.
  - [ ] Runtime elaboration-before-use checks and their cleanup regressions.
- [ ] **Elaboration pragmas.** Implement `Elaborate`, `Elaborate_All` and
  `Elaborate_Body`, replacing their Phase 1 rejections. Check placement,
  arguments and named-unit legality. Test each pragma separately: body-before-
  client ordering for `Elaborate`, the required transitive closure for
  `Elaborate_All`, and a required body immediately following its specification
  for `Elaborate_Body`. Diagnose conflicting constraints and carry the pragma
  requirements through scan metadata, binding and incremental rebuilds.
- [ ] **Visibility and conformance.** Audit visibility, `use` clauses,
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
- [ ] **Blocks executed during package-body elaboration.** Audit ordinary
  `declare ... begin ... end` and `begin ... end` statements inside package
  bodies' executable parts. Cover local declarations, object storage, nested
  subprogram emission and captures, local package elaboration, handlers and
  cleanup on both normal and exceptional exits. Check that local storage and
  controlled lifetimes end with the block rather than library shutdown.
- [ ] **Aggregates.** Completeness, duplicate and overlapping choices, `others`,
  record defaults, array sliding and component subtype checks beyond the
  existing regressions. Include null and multidimensional arrays, inferred
  bounds, default-expression evaluation counts and failure during component
  initialization.
- [ ] **Array conversions.** Static component-subtype matching, index conversions,
  bound sliding, view conversions and runtime checks. Include null arrays,
  multidimensional bounds and converted actuals for `out`/`in out` parameters,
  including copy-back checks. Retain and explicitly test the current cross-type
  restriction requiring identical component subtype objects; document any
  proposed relaxation as a separate scope decision before implementing it.
- [ ] **Derived types.** Audit derivation legality, inherited constraints and
  primitive operations, type identity and explicit conversion rules. Cover
  accepted and rejected conversions and runtime range, discriminant and tag
  checks where applicable to the supported subset.
- [ ] **Attributes.** `'Pos`, `'Val`, `'Succ`, `'Pred`, `'First`, `'Last` and
  `'Range`: base versus subtype rules, valid type/object prefixes, operand
  types, arity and result width. Include scalar endpoints, null ranges,
  multidimensional array dimension arguments, and static versus runtime
  failures within the existing numeric model.
- [ ] **Primitive operations.** Wider visibility and legality audit for tagged
  and untagged primitives: declaration placement, inheritance, hiding,
  overriding indicators, controlling profiles and dispatch through supported
  class-wide views. Include private views and separate compilation.
- [ ] **Variant records.** Confirm that nested variant parts get correct layout,
  initialization and selection checks at every level. Cover assignment,
  copying, aggregates and discriminant mismatches for supported constrained
  objects, retaining active-component equality regressions. Mutable
  discriminants and controlled variant components remain in Phase 6.
- [ ] **Feature interactions.** Add focused combinations of the audited features
  with existing overload resolution, named/defaulted arguments, generic
  instantiation, renaming, controlled ownership and exception handling. Check
  copy-in/copy-out behavior, owned results and partial initialization cleanup;
  preserve the Phase 2 master/context boundaries on normal and exceptional
  exits. Reuse existing regression coverage and add cases for uncovered
  interactions rather than duplicating entire feature suites.

Completion criteria for every item: record the supported behavior and any
remaining restrictions; add positive executable tests, legality rejection
tests and runtime-failure tests as applicable. Cover the cases named above,
including valid dependency patterns alongside cycle failures, malformed
elaboration manifests, each pragma's ordering and legality, calls before body
elaboration, and valid completions alongside each conformance mismatch.
Exercise whole-program and separate compilation for cross-unit behavior,
including rebuilds after changes to dependencies or elaboration pragmas.
Validate behavior against the Ada 95 baseline and selected Ada 2005 features,
accounting for documented implementation-defined behavior. Add determinism tests
whenever a change affects generated code, update README.md with changed behavior,
and keep the full suite green. Mark an item complete only when its audit and
applicable regression coverage are complete.

## Phase 4 — Minimal tasking
A working subset, close to the Ravenscar profile, that makes tasking a tested
part of the compiler early. Later phases must keep it working.

- [ ] **Tasks.** Task types and single task objects without entries; activation
  at the end of the declarative part or library elaboration; masters awaiting
  task objects as record components and allocated tasks.
- [ ] **Protected objects.** Protected types and single protected objects with
  procedures and functions, under one lock per object.
- [ ] **Protected entries.** Entries with barriers, re-evaluated when a
  protected procedure or entry completes.
- [ ] **Time.** `Duration` (an ordinary fixed-point type), `delay`,
  `delay until`, and `Ada.Real_Time`.
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
  generic formals and `Ada.Text_IO.Modular_IO`.
- [ ] **Ordinary fixed point.** Wider scale choices, explicit `Small` clauses,
  dynamic subtype bounds, remaining attributes and streaming.
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

Grouped by dependency. Within a group, work top to bottom.

### Dynamic data

- [ ] Runtime scalar constraints in anonymous subtype indications, library
  declarations, real subtypes, `'Width` and streaming.
- [ ] Library-level arrays and types with runtime bounds, including runtime-bound
  generic array objects (`runtimearraylibraryerrors.ads` lists the cases).
- [ ] Runtime-constrained array components and allocators, with `'Size` and
  streaming.

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
- [ ] Anonymous access types, including anonymous access-to-subprogram.
- [ ] `'Access` of imported subprograms and of inherited tagged operations.

### Program structure

- [ ] Separate subunits for subprogram and package bodies: loading, and
  analysis in the parent's scope. Replaces the Phase 1 diagnostic.
- [ ] Package and generic renaming (object, subprogram and exception renaming
  exist).
- [ ] Labels and `goto`, with legality checks for transfers between scopes.
- [ ] Extended return statements for results that are not controlled types.

### Object orientation

- [ ] Abstract subprograms and their full legality rules.
- [ ] Discriminated tagged types, private extensions, extensions of private
  views, general limited record definitions, and tagged size clauses.
- [ ] Interfaces, after abstract subprograms. Leave room in the dispatch tables
  for synchronized, task and protected interfaces (Phase 9).

### Generics

Code: `Parser::parseGenericDeclaration`, `Sema::bindGenericFormals`,
`Sema::analyzeGenericInstantiation`.

- [ ] Parse and enforce all formal type categories. The parser still skips much
  of the formal type definitions; add derived, access, modular and decimal
  formals as those types become available.
- [ ] Abstract formal subprograms, null procedure defaults, attribute and
  enumeration-literal actuals, and matching of runtime array constraints.
- [ ] Formal packages and matching of their generic contracts.
- [ ] Access formal objects, limited `in` objects built in place, and
  library-level runtime-bound array formals.
- [ ] Extend contract checking to each new formal category, and remove the
  remaining legality rechecks done at instantiation.

Tests: defaulted discriminants with variant changes, a callback capturing an
`aliased` local, an access value escaping its scope, subunits using enclosing
declarations, a formal package, an interface hierarchy with dispatch.

## Phase 7 — Predefined library

- [ ] `Ada.Strings.Unbounded`, written in Ada on the controlled-type support.
  Then a basic `Ada.Containers.Vectors`. Cover copying, growth, element
  lifetime, bounds and exception cleanup.
- [ ] Type-aware streaming: components, bounds and discriminants, user-defined
  stream attributes and a real stream abstraction. Streams mostly copy object
  representation today. Then add controlled and tag-aware class-wide streaming.
- [ ] `Ada.Exceptions`: expanded names (now uppercase defining identifiers),
  wide names, occurrence streaming and occurrence-valued function results.
- [ ] `Ada.Strings.Maps.Constants` and mapping-function callbacks, which
  access-to-subprogram support now permits.
- [ ] Remaining `Text_IO` and numeric input grammar cases.
- [ ] `Ada.Calendar`, on the Phase 4 `Duration`.
- [ ] Enforcement of package categorization (`Pure`, `Preelaborate`).
- [ ] `Wide_Character` and `Wide_String`, and a documented source encoding policy.
- [ ] A foreign ABI and ownership contract for imported controlled results.

## Phase 8 — Tooling and validation (alongside the phases above)

- [x] Record the existing language target: Ada 95 with selected Ada 2005
  features, without claiming full conformance or widening the scope. Track
  supported behavior and restrictions in README.md and progress in this roadmap.
- [ ] Robustness: malformed-source, fuzzing, crash and timeout coverage, and
  sanitizer builds. Bound the run time of the compiler and of test programs.
  From Phase 4, run the tasking tests under ThreadSanitizer.
  - [x] Record-component syntax recovery preserves record/variant boundaries;
    declaration recovery preserves the next declaration's introducer after a
    missing delimiter. Covered by `recordrecoveryaccess`,
    `recordrecoverymissingsemicolon` and `recordrecoveryerrors`, with timeouts.
- [ ] Adopt relevant Ada 95 and selected Ada 2005 conformance tests,
  allowing for documented implementation-defined behavior. GNAT comparisons
  are not a project requirement.
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

Completes the model on top of Phases 4 and 6.

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
