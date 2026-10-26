# Generic type formals — Phase 6g

The target is Ada 2005 RM [12.5.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-12-5-1.html)
and [12.5.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-12-5-4.html).
These increments extend the numeric, discrete, array and private formals;
Phase 6g remains in progress.

## Private actual subtypes

A definite private formal accepts constrained array subtypes, including runtime
bounds and multiple dimensions. Matching examines the actual subtype rather
than stripping its constraint through the base-type identity. Instance bindings
retain that subtype, so object initialization, copies, reference actuals, results
and sliding use the existing descriptor and ownership mechanisms.

`type T (<>) is private` and `type T (<>) is limited private` allow indefinite
actual subtypes. The checked contract retains the unknown-discriminant view:
uninitialized variable objects, array/record components and passing that view
to a definite formal are diagnosed before any instance is needed. A private
formal without discriminants also rejects an unconstrained array, a class-wide
subtype or a discriminated record lacking both constraints and defaults.
Limited actuals still require a limited formal.

## Access formals

Named object access formals support pool-specific, general variable and constant
access, and null exclusions. Named access-to-procedure/function formals support
the existing callback profiles. Matching enforces:

- Object versus subprogram access and statically matching designated subtypes.
- Constant access on both sides or neither; an `all` formal requires general
  variable access. A pool-specific formal can accept general variable access.
- Identical outer null exclusions.
- Mode-conformant designated callback profiles: parameter count, modes, types
  and result type. Parameter names and scalar subtype bounds may differ.

Each formal access definition is checked as a named type in the generic's
contract scope. A fresh definition is resolved for matching with preceding
formals bound to actuals and declaration-site names replayed from the contract.
These matching-only declarations stay owned by the instance but are not emitted
as new access collections. The instance binds the original actual type, reusing
its allocation collection, accessibility, callback convention and checks.
Contract violations such as writing through access-to-constant, arithmetic on a
private designated object or an invalid callback call are rejected even without
an instantiation.

## Derived and tagged formals

Formal derived declarations support existing scalar, array, record and named
object-access ancestors, including tagged private extensions and abstract
formals. The ancestor must be a subtype mark. Tagged ancestors require
`with private`; untagged ancestors forbid it. Inline constraints, full record
extensions, invalid abstract/limited modifiers and unrelated actual types are
diagnosed. Matching also retains inherited definiteness, scalar range
compatibility, array bounds, record discriminants and access null exclusions.
An abstract actual requires an abstract formal; each concrete operation promised
by an abstract formal must remain concrete for the actual.

An untagged formal's inherited operations denote the ancestor's implementations,
even when the actual overrides them. A tagged formal private extension instead
uses the actual's corresponding operations and tag. Profiles keep the formal
view's parameter names/defaults. A subtype view retains the actual's identity,
constraints, layout and ownership together with the formal's copied primitive
contract. Nested and separately compiled generic bodies inherit that contract,
so replay cannot substitute an unrelated actual override.

Tagged and abstract tagged private formals retain their category, abstractness,
limitedness and optional unknown discriminants during contract checking.
Tagged generic object views are aliased as required, including `in out` aliases
whose actual declaration does not say `aliased`.

Private overrides of inherited public primitives preserve a callable public
view with the inherited names/defaults. Private code sees the overriding
profile; external static calls, callbacks, dispatch and further derivations
reach the override through the public contract.

## Evidence and remaining work

`genericaccess`, `genericaccessmatching`, `genericprivatearrays`,
`genericderived`, `genericderivedtagged`, `genericderivednested`,
`genericderiveddiscriminants` and `genericderivedlibrary_main` run in whole,
per-unit and deterministic modes, including four concurrent workers.
The matching `*errors` tests cover syntax, category, contract, ancestry,
constraints, privacy and abstractness, with legal controls.

Unchanged CA2009A, CC54001–CC54004, C393013, C3A2A01, C640001 and C760010
execute successfully. C393013 covers the full null-extension controlling-result
scenario; C3A2A01 covers generic accessibility checks; C640001 exercises
callbacks and redispatch; C760010 covers controlled lifetime and exception
propagation through tagged private formals. Provenance and deferred cases are
in the [ACATS inventory](../tests/acats/README.md).

Validation passes all 1,176 registered tests (the 1,175-test full run plus the
subsequently registered CC54004), 17 focused compiler ASan/UBSan checks, and
five whole/per-unit execution scenarios with the instrumented C runtime.
The reviewed 123-case baseline is stable at 79 executable passes, 16 compile
rejections, eight harness blockers and 20 ungraded B observations.
LeakSanitizer is unavailable on this host; QBE-generated Ada memory accesses
remain uninstrumented.

Known discriminants on private formals, interface/synchronized formals,
interface ancestors/progenitors on derived formals, class-wide derived actuals,
Definite, formal packages and the later 6g bullets remain open. Class-wide
actuals for derived formals need the RM 12.5.1 dispatch wrappers and
controlling-result Program_Error behavior; they are diagnosed until implemented.
Existing limits on derivation before private completion and task/protected
composites still apply. Named protected callbacks retain Phase 9.
Generic access objects and limited `in` objects remain diagnosed. The predefined
Unchecked_Deallocation compatibility contract is unchanged.
