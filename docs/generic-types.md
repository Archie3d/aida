# Generic type formals — Phase 6g

The target is Ada 2005 RM [12.5.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-12-5-1.html),
[12.5.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-12-5-4.html)
and [12.5.5](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-12-5-5.html).
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

## Known discriminants and Definite

Private, limited-private and tagged-private formals can declare known
discriminants, including supported anonymous access discriminants. Defaults are
forbidden. The actual must be an unconstrained specific record subtype with
visible discriminants of matching count and statically matching subtypes. A
constrained subtype, hidden/unknown discriminant view, wrong subtype or null
exclusion is rejected. Discriminant types can refer to preceding formals.

Formal and actual discriminant names need not match. Checked selections and
named constraints retain discriminant positions through instance replay,
including nested generics and separate library bodies. An ordinary actual
field with the formal's discriminant name cannot capture a generic selection.
Instance clients still select the actual's own fields normally. Record subtype
membership checks discriminant constraints and supports `not in`.

`S'Definite` is supported for a formal indefinite subtype, including a subtype
alias. It returns whether the bound actual subtype is definite: constrained
arrays/records and defaulted records are definite; unconstrained arrays,
nondefaulted unconstrained records and class-wide subtypes are indefinite.
The attribute is nonstatic even when the instance's answer is known, and
cannot justify static case choices. Arguments, object prefixes, ordinary
subtypes and definite formals are diagnosed. The generic contract keeps the
formal indefinite even when a particular actual is definite.

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

## Interface formals

Formal interface declarations support ordinary, limited, synchronized, task and
protected interfaces, with zero or more progenitors. Actuals must be specific
interface types of exactly the same category and descend from every declared
progenitor. A concrete implementation or a class-wide subtype cannot stand in
for an interface actual. Duplicate and non-interface progenitors, incompatible
categories and unknown discriminants are diagnosed.

Derived formals can use ordinary/limited interfaces as ancestors or progenitors.
A nonlimited formal rejects a limited actual; a limited formal can accept a
nonlimited descendant of its limited interface ancestor. The existing abstract
and inherited-constraint rules still apply. Declaring a formal synchronized,
task or protected *interface* does not implement concurrent objects.
Synchronized derived formal views are supported separately below.

Corresponding primitives use interface-origin identities rather than assumed
slot numbers. Reordered actual interfaces, diamond inheritance, inherited null
procedures and matching concrete-ancestor implementations preserve the actual
layout, tags, lexical links and dispatch maps. Nested and separately compiled
instances retain the copied operation contract. Additional actual interface
operations still impose abstract overriding obligations when the instance
derives a concrete descendant; they cannot be silently dropped.

A generic contract can take Access of a nonabstract inherited formal operation
whose ancestor is abstract: the actual supplies the concrete implementation.
Contract checking records the name without marking the ancestor for closure
emission. Replaying an instance marks the actual callable entry instead.

## Synchronized derived formals

`type T is [abstract] synchronized new Parent [and Interfaces] with private`
retains a limited, synchronized tagged contract. Parent must be tagged and all
ancestors/progenitors must be limited; synchronized interface ancestry requires
`synchronized` rather than an omitted modifier or `limited`. Task and protected
interface ancestry cannot be combined. Actuals must descend from every promised
ancestor and be synchronized tagged types; abstract actuals still require an
abstract formal. The formal view is not itself an interface declaration.

The synchronized property survives subtype aliases and nested/library contract
replay. Existing primitive-origin matching preserves inherited null operations,
callbacks and dispatching call bodies. Unknown discriminants are allowed on a
formal private extension with an interface ancestor: the formal extension is a
record view, so the interface declaration's own discriminant restriction does
not prohibit this formal part. Scalar ancestors still reject discriminants.

`genericsynchronized`, `genericsynchronizednested` and
`genericsynchronizedlibrary_main` execute tag and inherited-callback identity
checks in whole/per-unit/deterministic modes with four workers; they also
compile inherited dispatching bodies. No concrete concurrent implementation is
created or invoked. `genericsynchronizederrors` isolates invalid modifiers,
ancestry conflicts, nonsynchronized/wrong-category actuals, abstractness and
attempts to implement a synchronized formal with an ordinary record extension.
Concrete task/protected implementations and executing those dispatching bodies
remain Phase 9. Class-wide derived actuals remain the next open 6g item.

## Evidence and remaining work

`genericaccess`, `genericaccessmatching`, `genericprivatearrays`,
`genericderived`, `genericderivedtagged`, `genericderivednested`,
`genericderiveddiscriminants`, `genericderivedlibrary_main`,
`genericinterfaceformals`, `genericinterfacecategories`, `genericinterfacenested`,
`genericinterfacelibrary_main`, `genericdiscriminants`, `genericdefinite`,
`genericdiscriminantviews`, `genericdiscriminantlibrary_main`,
`genericsynchronized`, `genericsynchronizednested` and
`genericsynchronizedlibrary_main` run in whole,
per-unit and deterministic modes, including four concurrent workers.
The matching `*errors` tests cover syntax, category, contract, ancestry,
constraints, privacy and abstractness, with legal controls.

Unchanged CC1204A, CC3240A, CA2009A, CC54001–CC54004, C393013, C3A2A01,
C640001 and C760010 execute successfully. C393013 covers the full null-extension
controlling-result scenario; C3A2A01 covers generic accessibility checks; C640001 exercises
callbacks and redispatch; C760010 covers controlled lifetime and exception
propagation through tagged private formals. Provenance and deferred cases are
in the [ACATS inventory](../tests/acats/README.md).

Current validation passes 1,227/1,227 tests, 14 focused compiler ASan/UBSan
checks and three synchronized-formal scenarios in whole/per-unit modes with the
instrumented C runtime. Unchanged B394A03–B394A05 add three ungraded ancestry
probes. B394A02 loses two obsolete formal-syntax parser errors; all other 124
previous observations are unchanged. Their full sources still reject unsupported
concurrent/private-extension constructs, so none is an upstream legality pass.
The 128-case baseline contains 79 executable passes, 16 compile rejections,
eight harness blockers and 25 ungraded B probes.
LeakSanitizer is unavailable on this host; QBE-generated Ada memory accesses
remain uninstrumented.

The remaining scope and completion criteria are listed in
[TODO.md, Phase 6g](../TODO.md#6g-generics). The longer checklist unpacks the
original five commitments rather than adding new feature families:

| Original commitment | Remaining work within it |
| --- | --- |
| Formal type categories | Class-wide derived actuals (synchronized derived contracts now implemented) |
| Formal subprograms and array constraints | Abstract formals; null defaults; attribute and enumeration-literal actuals; nonstatic constraint matching |
| Formal packages | Package contract matching, including partial lists and box associations |
| Formal objects | Access objects; limited `in` objects |
| Contract checking | Correct checking/replay for those features, not a separate architecture rewrite |

The scope audit compared the original roadmap at commit `094b812` (before the
first 6g increment) with `ParserGenerics.cpp`, `SemaGenerics.cpp`, existing
rejection tests and seven small compiler probes. At that audit the probes confirmed missing synchronized-
derived and abstract syntax, null defaults, attribute/enum actuals, nonstatic
formal arrays and formal packages. Synchronized-derived contracts have since
been implemented as described above. Access/limited-object restrictions remain
explicit in
`generic_object_errors`/`generic_composite_errors`; class-wide derived actuals
remain covered by CC51B03's exact rejection. Partial package associations are
part of the same missing formal-package feature.

Three qualifications prevent the checklist from overstating the work:

- Synchronized-derived syntax and contract matching are 6g work
  ([RM 12.5.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-12-5-1.html)).
  Full concurrent implementations/execution stay in Phase 9; they do not block
  completion of static support in 6g.
- Formal array constraints require static matching even when bounds are
  nonstatic. Preserve the identities allowed by
  [RM 4.9.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-9-1.html)
  and [RM 12.5.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-12-5-3.html);
  do not substitute runtime equality of unrelated dynamic ranges.
- Contract checking must retain the instance legality checks required by
  [RM 12.3(11)](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-12-3.html)
  while preserving template interpretation under 12.3(14). Removing all
  instantiation-time legality checking would be incorrect.

CC51B03 remains an exact compile XFAIL until class-wide derived actuals include
the RM 12.5.1 dispatch wrappers and controlling-result `Program_Error` behavior;
its full upstream Definite scenario is not counted as a pass.

The same roadmap explicitly lists generic-related Phase 8 work (prefixed and
implicit operator callable views, renaming-as-body completions and the broader
legality/staticness audit) and Phase 9 dependencies (concurrent implementations,
protected-operation actuals, named protected callbacks and task/protected
features). Existing limits on derivation before private completion and
composites still apply. Generic access objects and limited `in` objects remain
diagnosed. The predefined Unchecked_Deallocation compatibility contract is
unchanged.
