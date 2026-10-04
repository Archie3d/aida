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
The main items are done; the follow-ups below close the remaining silent paths.

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

Tests: extend `importconventionerrors` and `subprogramstuberrors`, plus an
executable test if `Import` defaults the external name.

## Phase 2 — Tasking foundations

Prepare the run time and code generator for several threads of control before
more features depend on single-threaded behavior. Nothing here adds Ada syntax;
the existing suite must pass unchanged, with no measurable slowdown.

Current state: generated code loads one run-time global directly
(`$__ada_exception`, in `QbeEmitter::emitExceptionCheck` and handler dispatch).
Finalization chains and storage checkpoints already live in stack frames, and
exceptions propagate by explicit checks rather than unwinding, so neither needs
redesigning. The shared state is in the C run time.

- [ ] **Per-task context.** Gather the mutable run-time state into one context
  record: pending exception, message and trace (`adart.c`), the current trace
  frame, and the registries of finalizations and allocations. The main program
  owns the first context; each task will own another.
- [ ] **Generated access to the context.** QBE's `thread` data cannot serve
  every target: it is rejected for extern symbols on arm64 Linux and entirely on
  Windows amd64 (`qbe/arm64/emit.c`, `qbe/amd64/emit.c`). Have each subprogram
  fetch the context pointer once on entry (a run-time call or a hidden
  parameter), and load the pending exception at a fixed offset from it. Measure
  the cost and pick one ABI before more emitter code reads run-time state.
- [ ] **Result buffers.** `'Image`, fixed-point text and stream helpers return
  pointers into rotating static buffers (`IMAGE_BUFFERS`,
  `ADA_STREAM_BUFFERS`). They are not task-safe, and they limit how many results
  one expression can combine. Return the results in caller-owned temporary
  storage, as unconstrained function results already are.
- [ ] **Shared run-time tables.** Protect the file table and current input/output
  (`adaio.c`), tag registrations (`adatags.c`) and library finalization with a
  run-time lock, or give them a single owner.
- [ ] **Threading layer.** Put a thin C layer over POSIX threads (winpthreads on
  MSYS2) for threads, mutexes, condition variables and monotonic clocks. It also
  supplies the atomic loads and stores that QBE lacks, for `Atomic` and
  `Volatile` objects.
- [ ] **Masters and activation hooks.** Every master (subprogram, block, library
  level, access-type collection) gets an exit point that will wait for its
  dependent tasks before finalizing objects, in the same place the finalization
  chain is unwound. Elaboration gets the matching activation point. These hooks
  do nothing until Phase 4, but later features are built around them.

Tests: the existing suite; a C test that runs run-time helpers (images,
exceptions, allocation) from several threads at once; IR golden updates for the
new context access.

## Phase 3 — Harden the existing subset

Audits of supported features. Write a focused reproduction before choosing a fix.

- [ ] **Visibility and conformance.** Visibility, `use` clauses, homographs,
  duplicate declarations, and specification/body conformance (parameter modes,
  names, defaults, return profiles). Report missing bodies before linking.
  Code: `adac/sema/SemaNames.cpp`, `adac/sema/SemaDecl.cpp`, `adac/Scope.cpp`.
- [ ] **Unit loading and elaboration.** Dependency cycles, explicit-source and
  spec/body loading, and elaboration-before-use checks. The binder currently
  trusts the manifest's order; make it check that order against `with`
  dependencies. Then implement `Elaborate`, `Elaborate_All` and `Elaborate_Body`,
  which Phase 1 currently rejects. Code: `adac/UnitLoader.cpp`, `adac/Binder.cpp`.
- [ ] **Library-level statement blocks.** Support declarations inside them,
  including object storage, nested subprogram emission and package elaboration.
- [ ] **Aggregates.** Completeness, duplicate choices, record defaults, array
  sliding and component subtype checks beyond the existing regressions.
- [ ] **Array conversions.** Static component-subtype matching, index conversions,
  bound sliding, view conversions and runtime checks. The supported cross-type
  subset requires identical component subtype objects.
- [ ] **Attributes.** `'Pos`, `'Val`, `'Succ`, `'Pred`, `'First`, `'Last` and
  `'Range`: base versus subtype rules, operand types, arity and result width.
- [ ] **Derived types.** Remaining derivation legality and conversion rules.
- [ ] **Primitive operations.** Wider visibility and legality audit for tagged and
  untagged primitives.
- [ ] **Variant records.** Confirm that nested variant parts get correct layout,
  initialization and selection checks at every level (equality is covered).

Tests: invalid specification/body pairs, mutually dependent units, initializers
depending on an unelaborated body, duplicate aggregate choices, invalid
attribute arguments.

## Phase 4 — Minimal tasking

A working subset, close to the Ravenscar profile, that makes tasking a tested
part of the compiler early. Later phases must keep it working.

- [ ] **Tasks.** Task types and single task objects without entries; activation
  at the end of the declarative part or library elaboration; masters awaiting
  termination; an unhandled exception terminating only its task. Includes
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

- [ ] Choose the target language version. Keep a support matrix separating
  parsed, semantically checked and correctly emitted constructs.
- [ ] Robustness: malformed-source, fuzzing, crash and timeout coverage, and
  sanitizer builds. Bound the run time of the compiler and of test programs.
  From Phase 4, run the tasking tests under ThreadSanitizer.
- [ ] Compare shared programs with GNAT, and adopt relevant conformance tests,
  allowing for implementation-defined differences.
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
