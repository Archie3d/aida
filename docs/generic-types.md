# Generic type formals — Phase 6g

The target is Ada 2005 RM [12.5.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-12-5-1.html),
[12.5.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-12-5-3.html),
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

## Access formal objects

Named and anonymous access-to-object and access-to-subprogram formal objects
support modes `in` and `in out`, defaults for `in`, and null exclusions. An `in`
formal is a constant initialized once at instance elaboration: it copies the
access value, including callback closure and accessibility metadata, and retains
the designated object's identity. An `in out` formal aliases the actual access
object; assigning a new access value changes that object. Repeated actuals keep
their aliases, and indexed actual names are evaluated once.

Named formals require the same access type. Anonymous formals require statically
matching designated subtypes and identical access-to-constant properties, or
subtype-conformant designated subprogram profiles and matching conventions.
Unlike ordinary anonymous access initialization, an anonymous formal object
cannot accept a variable-access object as a constant-access actual. An anonymous
`in out` formal additionally requires an anonymous access object actual. Null,
constants and expressions cannot serve as variable actuals.

An explicit null exclusion on an `in out` formal requires an actual whose subtype
excludes null; forwarding another generic formal in a generic body requires its
explicit exclusion. Mode `in` performs a checked initialization, including for
nullable actuals and defaults. This uses the correction in
[AI12-0287-1](https://www.adaic.org/resources/add_content/standards/22aarm/html/AA-12-4.html),
which limits the static null-exclusion matching rule to mode `in out`; the
original [Ada 2005 RM 12.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-12-4.html)
wording imposed it on both modes. The existing unchanged C3A0025 scenario
requires the checked `in` behavior. A named null-excluding subtype also retains
its initialization check when used as a formal without an explicit exclusion.

Existing object/subprogram accessibility checks apply to the actual and to uses
inside the template. Borrowed access parameters may feed an instance within the
call; a deeper designation cannot escape into an enclosing anonymous access
object. Anonymous allocator collections belong to the instance's enclosing master and clean up
controlled designations after normal completion, exceptions and failure of a
later actual. Defaults replay declaration-site resolution, including through
nested and separate library generic bodies.

`genericaccessobjects`, `genericanonymousobjects`, `genericaccesscallbacks`,
`genericaccesslifetime` and `genericaccessobjectlibrary_main` execute whole/per-unit
and determinism checks with four workers. `genericaccessobjecterrors` checks
variable views, named/anonymous distinctions, designated subtype/profile
mismatches, defaults, null exclusions and invalid escaping/forwarding contracts
without an instance. Unchanged ACATS C3A0020 and C3A0025 now execute as PASS;
the old rejection snapshots are retired. Compiler/C-runtime ASan/UBSan checks
cover all five scenarios in whole/per-unit modes, the rejection test and both
ACATS executions. Generated QBE memory accesses remain uninstrumented;
LeakSanitizer is unavailable in this environment.

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
remain Phase 9.

## Class-wide derived actuals

A tagged derived formal with unknown discriminants accepts class-wide actual
subtypes whose specific root descends from the ancestor and every progenitor.
Definite formals continue to reject indefinite class-wide subtypes. Matching
retains the actual subtype's constraints, identity and ownership; a class-wide
root may be abstract because dispatch reaches a concrete descendant.

Copied inherited operations act as intrinsic dispatch wrappers, preserving the
formal parameter names and defaults while calling the corresponding actual
operation. They use the existing tag checks, interface-origin mapping, lexical
links and owned-result ABI. Controlling access parameters dispatch through the
designated object's tag. Multiple controlling operands must have the same tag,
otherwise `Constraint_Error` is raised. Nested formal ancestors retain their
copied contracts when an enclosing formal is bound to a class-wide actual.

For an operation with a controlling result and no controlling parameters,
a class-wide assignment or surrounding dispatch supplies the concrete tag.
A call whose tag would instead be statically that of the formal type raises
`Program_Error`. Any call through a renaming of that operation raises
`Program_Error`, including within an assignment with a concrete destination tag
(RM 12.5.1(23.1–23.3)). Inherited operations of tagged formals with unknown
discriminants are intrinsic already in the template (RM 6.3.1(8)); taking their
`Access` is diagnosed without requiring an instance.

`genericclasswide`, `genericclasswideinterfaces` and
`genericclasswidelibrary_main` cover overrides, formal profile replay, abstract
roots, reordered interface maps, anonymous controlling access parameters,
controlling results, default arguments, tag mismatch, nested instances and
library bodies. Each executes whole/per-unit and deterministic checks with four
workers. `genericclasswideerrors` and `genericderivederrors` check ancestry,
progenitors, definiteness, illegal class-wide ancestors and intrinsic access
restrictions, with legal controls.
Unchanged CC51B03 and its FC51B00 foundation execute the full Definite scenario
as PASS; their former compile-XFAIL expectation is removed.

## Abstract formal subprograms

`with procedure ... is abstract [Default];` and
`with function ... return ... is abstract [Default];` support an omitted default,
a declaration-site name, or `<>`. An abstract formal has exactly one specific
tagged controlling type, considering its parameters, result and anonymous
access designated types (RM 12.6(8.4)). Class-wide and named access types do not
supply a controlling type. Null defaults remain forbidden for abstract formals.

An actual must have a mode-conformant profile and denote a dispatching operation
of the controlling type or its corresponding actual type (RM 12.6(8.5)). It may
be abstract or concrete; a renaming of a dispatching operation is also accepted.
Named defaults check explicit null exclusions, resolve before the formal becomes visible and replay the
checked declaration-site binding. Box defaults resolve at the instantiation
site. Invalid named defaults are rejected even without an instance.

The copied formal remains abstract even for a concrete actual, so every call
must dispatch. Specific statically tagged calls and `Access` are diagnosed in
uninstantiated templates. An instance retains the actual's parameter/result
subtypes and dispatch slot, with the formal's names and default expressions.
The formal is a dispatching view and does not add or replace a primitive slot
on the controlling type. Inherited homographs are hidden within the formal
scope while the type's original primitive contract remains intact.

Dispatch covers controlling access parameters, tag-indeterminate results,
interfaces, nested abstract-formal actuals, and separately compiled library
bodies. A formal operator dispatches to its supplied operation, including when
formal `"="` denotes a differently named comparator; implicit `"/="` negates that
same dispatched result. The existing tag consistency and result ownership
checks still apply.

`genericabstract`, `genericabstractinterfaces`, `genericabstractoperators` and
`genericabstractlibrary_main` execute whole/per-unit/deterministic checks with
four workers. Matching `*errors` cover missing/multiple controlling types,
nonprimitive actuals, mode mismatches, invalid defaults, static calls and
abstract `Access`, including uninstantiated invalid templates and legal controls.
Unchanged Ada 2005 ACATS CC60001 executes its complete dispatch/box-default
scenario as PASS. The abstract-formal increment passed 1,255/1,255 tests. Four scenarios also pass compiler/runtime ASan/UBSan execution
in whole/per-unit modes; four rejection tests and CC60001 pass compiler checks.
Generated Ada memory accesses remain uninstrumented, and LeakSanitizer is
unavailable on this host.

## Null procedure defaults

`with procedure ... is null;` permits an omitted actual, including a procedure
without parameters. The synthesized actual is an intrinsic null procedure with
the formal profile (RM 12.6(10.1/2,16.2/2)). Functions and abstract formals cannot
use a null default. An explicitly supplied subprogram still undergoes the usual
mode/profile/null-exclusion matching and keeps its own calling convention;
a null default does not perform name lookup at the instantiation site.

Calls use the ordinary call ABI with an empty null-procedure body. Actual and
parameter-default expressions are evaluated once at the call, with range,
array-shape and null checks, exception propagation and normal temporary cleanup.
Limited and unconstrained composite parameters retain their existing reference
and descriptor behavior. Concrete formal subprograms hide inherited homographs
in the formal scope without becoming new primitives or dispatching operations.

The intrinsic convention survives public renamings, forwarding through another
generic formal, and nested/separate library replay. Taking `Access` of a view
ultimately bound to the intrinsic default is rejected; a replacement with an
ordinary Ada procedure remains callable through `Access`.

`genericnull`, `genericnullcomposites` and `genericnulllibrary_main` execute
whole/per-unit/deterministic checks with four workers. They cover empty and
parameterized profiles, omitted/explicit actuals, side effects and exceptions
in arguments/defaults, checks, aliases, limited records, slices, closures and
nested/library forwarding. `genericnull*errors`, `nullformaldefaulterrors` and
`genericabstractsyntaxerrors` reject invalid actuals, illegal defaults, intrinsic
Access and attempts to use concrete formals as dispatching operations.
Unchanged BC60001 is a frozen B_REJECTED_UNGRADED observation; its four forbidden
default syntax errors prevent semantic grading of the rest of that source.
Local tests separately exercise its legal controls and intrinsic restriction.
The prior 128 baseline observations were unchanged; at that increment the cohort had
79 executable passes, 16 compile rejections, eight harness blockers and 26
ungraded B observations.

Null-default validation passes 1,268/1,268 tests. Three scenarios execute
whole/per-unit with the compiler and C runtime under ASan/UBSan; five rejection
tests and the BC60001 observation also pass compiler instrumentation. Generated
Ada memory accesses remain uninstrumented; LeakSanitizer is unavailable here.

## Attribute subprogram actuals

Scalar `Image`, `Value`, `Succ`, `Pred`, `Min` and `Max` can be supplied explicitly
or as named defaults to concrete formal functions. Supported prefixes include
integer, modular, enumeration/character, floating, ordinary fixed and decimal
subtypes, including selected names and `Base` prefixes. Box defaults can find
an existing attribute renaming by the formal's name.

Resolution constructs the attribute's callable profile and requires mode
conformance with the formal. Image/Value use unconstrained String; scalar
parameters/results use the base subtype, even when the prefix or formal names
a constrained subtype. A forwarded callable view also retains the actual's
subtypes and parameter ABI, with the formal's names and default expressions.
Attribute defaults are checked without an instance and retain declaration-site
prefix bindings during nested/library replay. Generated calls use a bound type
mark so parameter names and instantiation-site hiding cannot capture the prefix.
The shared direct-attribute lowering and normal result ownership handle
argument evaluation, exceptions and returned strings. Intrinsic convention
survives renaming/forwarding and prevents `Access` of the bound view.

Two local prerequisites are included: scalar Min/Max now lower for signed,
enumeration and floating types as well as the existing modular/fixed cases;
formal scalar First/Last do not fold contract placeholder bounds or exact-value
metadata into false overflow diagnostics. Instance bounds are still evaluated
and checked normally. `genericattributestaticerrors` confirms those template
bounds cannot be used as static case choices.

`genericattributes`, `genericattributenumeric` and
`genericattributelibrary_main` run whole/per-unit/deterministically with four
workers. They cover explicit/default/box binding, base profiles, selected/Base
prefixes, parameter names/defaults, nested forwarding, prefix hiding, single
argument evaluation and numeric families including binary32/64. Matching
`*errors` reject mismatched profiles, private/object prefixes, attribute calls
instead of references, non-callable attributes and intrinsic Access.

Unchanged CC1302A executes its full nested default-attribute scenario as PASS.
CC3601A retains an exact compile XFAIL for grouped formal object declarations
such as `V, V1 : T`; this separate Phase 8 grammar gap prevents execution of its
whole operator/attribute scenario. All 129 prior backfill observations are
unchanged. Current validation passes 1,282/1,282 tests; three local scenarios
pass compiler/runtime ASan/UBSan in whole/per-unit modes, three rejection cases
pass compiler checks and CC1302A executes with the instrumented C runtime.
QBE-generated Ada memory accesses remain uninstrumented; LeakSanitizer is
unavailable on this host.

Attributes with anonymous universal profiles (`Pos`, `Val`, `Mod`, decimal
`Round`) cannot conform to a named subprogram profile; this is the same
restriction described for renamings in RM 8.5.4(10). Attribute values such as
First/Last/Size are not parameterless function actuals. Other currently
unsupported scalar functions retain diagnostics. Standard stream-attribute
actuals await Phase 7's Root_Stream_Type/class-wide stream profile model; the
current opaque stream-address ABI is not substituted for that profile.

## Enumeration-literal subprogram actuals

Enumeration identifiers and character literals denote parameterless functions
when supplied to a formal function with a matching result type (RM 3.5.1, 12.6).
Expanded names such as `Pkg.Red`, `Pkg.'A'` and `Standard.'a'` preserve visibility,
overloads and character case. Objects, numeric values and qualified expressions
are not callable actuals. Procedures, nonempty parameter profiles and unrelated
result types do not match.

Explicit actuals, named defaults and box defaults are supported. Literal named
defaults are checked without an instance and retain their declaration-site
binding through nested and separate library replay, even if the client hides
the name. A same-named formal can hide an inherited literal after resolving its
default. Inherited literal results retain the derived base subtype, including
values outside the derived first subtype's constraint. Calls preserve
positional values under enumeration representation clauses.

The bound view uses Intrinsic convention through renaming and forwarding;
`Access` is rejected. Explicit ordinary Ada function replacements retain their
own convention. Calls use the normal result-checking path. Static conversions
between related enumeration types retain their positions and constraint checks,
so they can appear in case choices.

`genericliterals` and `genericliterallibrary_main` exercise whole/per-unit and
deterministic builds with four concurrent workers. `genericliteralerrors`
rejects invalid actuals/defaults and intrinsic Access through forwarded aliases.
Unchanged C540001 executes its complete case-expression scenario as PASS and
joins the normal ACATS label. It is the only changed observation in the 129-case
baseline: 80 executable passes, 15 compile rejections, eight harness blockers
and 26 ungraded B observations.

Validation passes 1,290/1,290 tests and the complete baseline snapshot check.
Both local scenarios pass whole/per-unit execution with the compiler and C
runtime under ASan/UBSan; the rejection test and unchanged C540001 also pass
instrumented compiler checks, with C540001 executing against the instrumented
C runtime. Generated Ada memory accesses remain uninstrumented; LeakSanitizer
is unavailable on this host.

## Nonstatic constraints in formal array matching

Constrained and unconstrained formal arrays accept nonstatic index and component
constraints when they statically match (RM 12.5.3 and 4.9.1). Shared scalar
subtypes and their aliases retain the identity of the original constraint.
A constrained array records that identity separately for each index; its runtime
bounds descriptor still controls storage, indexing, attributes, copies and
results. An actual with a descriptor is constrained for matching even though its
storage layout is determined at runtime.

For constrained formals, index ranges must share the same constraint evaluation
or have equal static bounds. For unconstrained formals, the index subtypes must
statically match. Component subtypes also statically match, including shared
runtime scalar ranges, constrained array components and record discriminants.
Aliases and derivations preserve index metadata. Rank, index type, constrained
category, component subtype and aliased-component requirements still apply.

An array subtype constraint can name a scalar subtype, as in `Vector (Index)`.
Its generated bounds read the existing subtype's captured values and preserve
the source mark for contract replay. This differs from a new range such as
`Vector (Low .. High)`: independently elaborated nonstatic constraints remain
distinct even when their expressions or runtime values are equal. Matching
never substitutes a runtime equality test for the required static rule.

`genericarrayconstraints`, `genericarraycomponents` and
`genericarrayconstraintlibrary_main` run whole/per-unit/deterministically with
four workers. Coverage includes aliases, derived arrays, multiple dimensions,
null ranges, single evaluation of bound functions, dynamic component layouts,
returned arrays and nested/library instances. `genericarrayconstrainterrors`
rejects independent index/range/component constraints with coincident values,
constrained/unconstrained mismatches and a mismatching second dimension.
All 129 upstream baseline observations remain unchanged; no new ACATS pass is
claimed for these local scenarios.

Validation passes 1,300/1,300 tests. All three executable scenarios pass
whole/per-unit with the compiler and C runtime under ASan/UBSan; the rejection
test also passes compiler instrumentation. Generated Ada memory accesses remain
uninstrumented, and LeakSanitizer is unavailable on this host.

## Formal packages and contract matching

`with package P is new Template (<>);` declares a fully boxed formal package.
Complete positional/named actual lists and omission of defaulted actuals are
also supported. The template must be a generic package, and the actual must
be an instance of that template, including through package/generic renaming
and forwarding of another formal package (RM 12.7).

A boxed view exposes the template's visible declarations plus copies of its
formals and their implicit primitive declarations. Each formal package receives
a distinct checked specification: `P.T` and `Q.T` do not become the same type
merely because P and Q have the same template. Private declarations remain
hidden. Complete lists expose the package's visible declarations but do not
expose the template formals that are required to match. Generic-formal `use`
and `use type` clauses apply at their source position and into the unit body.

Complete contracts check every actual, including defaults. Type actuals must
statically match; `in` objects require equal static values, the same statically
named constant, or null; other objects, subprograms and packages require the
same entity. Generic `in` object actuals are followed recursively. Renamed
operations retain their identity; intrinsic operators, enumeration literals,
attribute functions and null defaults retain their callable identity. Static
string matching handles literals, constants, concatenation, qualification,
conversion and slices without evaluating arbitrary functions at compile time.
Independently declared equal-valued runtime constants and coincident variable
values do not satisfy the contract.

A bound view maps checked members, nested package members and copied formals
to the existing actual instance. Matching-only specifications emit no package
elaboration: actual package state, exceptions, closures and object identity
are reused. Symbolic formal-array index bounds remain symbolic until an actual
subtype is available; subtype-mark array aggregate choices lower to their
bounds. These prerequisites allow the complete CC70002 matrix/group scenario.

`genericpackages`, `genericpackagecontracts`, `genericpackagecallables` and
`genericpackagelibrary_main` exercise whole/per-unit/deterministic builds with
four workers, including distinct instances, forwarding, nested packages,
defaults, static values, shared state, single elaboration and exception identity.
Three rejection tests cover wrong templates/actuals, independent types,
private members, hidden matched formals and invalid associations.
Unchanged CC70002, CC70003, CC70A02 and CC70B01 execute as PASS with their
unchanged FC70A00/FC70B00 foundations. All 129 prior baseline observations remain
unchanged. Source hashes and grading boundaries are in the
[ACATS inventory](../tests/acats/README.md#formal-packages-phase-6g).

The core formal-package increment passed 1,319/1,319 tests and the complete baseline snapshot check.
All four local scenarios execute whole/per-unit with the compiler and C runtime
under ASan/UBSan. The three rejection tests and four unchanged ACATS scenarios
also pass compiler instrumentation; those ACATS executables run against the
instrumented C runtime. Generated Ada memory accesses remain uninstrumented;
LeakSanitizer is unavailable on this host.

CC70001 remains deferred because its generic child package of a generic parent
is rejected by the existing unit loader before formal-package execution; this
source does not establish a PASS.

## Partial formal-package actual lists and boxes

Ada 2005 mixed lists support explicit positional/named associations, named
`Formal => <>` associations and a final `others => <>`. The standalone
`(others => <>)` form has the same unrestricted matching as `(<>)`. A positional box is not permitted. Selectors must identify a
unique formal; duplicates, unknown selectors, excess actuals, positional
associations after named ones and misplaced/non-box `others` are diagnosed.

A partial view checks explicit actuals using the ordinary generic binding
rules, in formal declaration order. The remaining parameters keep copied
formal declarations, including dependent type definitions and callable profiles.
Only the parameters required to match constrain the supplied instance. A mixed
explicit/boxed list leaves omitted defaulted parameters unmatched; without
`others => <>`, those omissions must still have defaults. A list containing
only named boxes leaves omitted defaulted parameters required to match their
defaults. Complete lists keep the existing rule that every actual, including
defaults, must match (RM 12.7(4.1,5.1–5.2)).

Unmatched formals and their implicit declarations are visible through the view;
matched formals and private declarations remain hidden. This includes implicit
`/=` for a copied `=` formal. Boxed types in different views remain independent.
For overloaded formal subprograms, positional actuals and trailing `others`
preserve the identity and visibility of each declaration; an ambiguous named
selector is rejected. Each view starts from the actual package specification,
so forwarding a more permissive formal package cannot leak its extra copied
formals into a more restrictive view.

Partial views can depend on earlier packages (for example, one package's value
type as another's explicit key type), can themselves be package actuals, and
replay through nested and separately compiled generics. Member mappings from
an earlier package binding also apply when binding later type/object/subprogram
profiles. Checking a view does not elaborate another runtime package instance
or evaluate boxed defaults; the actual instance's state and elaboration remain
shared.

`genericpackagepartial`, `genericpackagepartialtypes`,
`genericpackagepartialoverloads` and `genericpartialpackagelibrary_main` exercise
whole/per-unit/deterministic builds with four workers. The rejection tests cover
matching, visibility, defaults, forwarding, ambiguous overload selectors and
association syntax. CC70002/CC70003/CC70A02/CC70B01 remain unchanged executable
PASS cases; the 129-case observational baseline remains unchanged. No new
partial-specific upstream PASS is claimed. The pinned archive's CC51009 and
CC51011 examples use Ada 2012 formal incomplete types (CC51011 also cites a
later AI12 correction), so they are excluded from the Ada 2005 selection.

Validation passes 1,333/1,333 tests. All four new scenarios execute whole/per-unit
with the compiler and C runtime under ASan/UBSan; five rejection checks pass
compiler instrumentation. The four existing formal-package ACATS scenarios also
pass instrumented compiler/C-runtime execution. Generated Ada memory accesses
remain uninstrumented; LeakSanitizer is unavailable on this host.


## Evidence and remaining work

`genericaccess`, `genericaccessmatching`, `genericprivatearrays`,
`genericderived`, `genericderivedtagged`, `genericderivednested`,
`genericderiveddiscriminants`, `genericderivedlibrary_main`,
`genericinterfaceformals`, `genericinterfacecategories`, `genericinterfacenested`,
`genericinterfacelibrary_main`, `genericdiscriminants`, `genericdefinite`,
`genericdiscriminantviews`, `genericdiscriminantlibrary_main`,
`genericsynchronized`, `genericsynchronizednested` and
`genericsynchronizedlibrary_main`, `genericclasswide`,
`genericclasswideinterfaces` and `genericclasswidelibrary_main` run in whole,
per-unit and deterministic modes, including four concurrent workers.
The matching `*errors` tests cover syntax, category, contract, ancestry,
constraints, privacy and abstractness, with legal controls.

Unchanged CC1204A, CC3240A, CA2009A, CC54001–CC54004, C393013, C3A2A01,
C640001, C760010, CC51B03, CC60001, CC1302A and C540001 execute successfully. C393013 covers the full null-extension
controlling-result scenario; C3A2A01 covers generic accessibility checks; C640001 exercises
callbacks and redispatch; C760010 covers controlled lifetime and exception
propagation through tagged private formals. Provenance and deferred cases are
in the [ACATS inventory](../tests/acats/README.md).

The class-wide actual increment passed 1,238/1,238 tests. Five scenarios pass
whole/per-unit execution with the compiler and C runtime under ASan/UBSan;
two rejection tests and unchanged CC51B03 also pass compiler instrumentation.
The synchronized-formal increment previously passed 14 focused compiler checks
and three scenarios with the instrumented C runtime. Unchanged B394A03–B394A05 add three ungraded ancestry
probes. B394A02 loses two obsolete formal-syntax parser errors; all other 124
previous observations are unchanged. Their full sources still reject unsupported
concurrent/private-extension constructs, so none is an upstream legality pass.
At that increment the 128-case baseline contained 79 executable passes, 16 compile rejections,
eight harness blockers and 25 ungraded B probes.
LeakSanitizer is unavailable on this host; QBE-generated Ada memory accesses
remain uninstrumented.

The remaining scope and completion criteria are listed in
[TODO.md, Phase 6g](../TODO.md#6g-generics). The longer checklist unpacks the
original five commitments rather than adding new feature families:

| Original commitment | Remaining work within it |
| --- | --- |
| Formal type categories | Supported derived categories implemented, including synchronized contracts and class-wide actuals |
| Formal subprograms and array constraints | Implemented for the documented profiles and matching rules |
| Formal packages | Implemented for the documented template/view categories; generic-child loader limitation remains |
| Formal objects | Access objects; limited `in` objects |
| Contract checking | Correct checking/replay for those features, not a separate architecture rewrite |

The scope audit compared the original roadmap at commit `094b812` (before the
first 6g increment) with `ParserGenerics.cpp`, `SemaGenerics.cpp`, existing
rejection tests and seven small compiler probes. At that audit the probes confirmed missing synchronized-
derived and abstract syntax, null defaults, attribute/enum actuals, nonstatic
formal arrays and formal packages. Synchronized-derived contracts, abstract formals,
null defaults, attribute/literal actuals, nonstatic formal-array matching and core formal packages have since been implemented
as described above. Access formal objects are now implemented; limited-object
restrictions remain explicit in `generic_composite_errors`. Class-wide derived actuals
now pass the full CC51B03 scenario. Partial package associations and boxes are now implemented as described above.

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

CC51B03 now executes unchanged as PASS. Local `genericclasswide*` additionally
checks RM 12.5.1 dispatch wrappers and controlling-result `Program_Error`
behavior beyond the upstream Definite scenario.

One reproduced replay gap remains owned by the final 6g contract-checking item:
when an outer derived formal is bound to `Root'Class`, an inner abstract-formal
instance in its body using that formal and its inherited operation currently
rejects bound `T'Class` and the controlling profile. This is the nested-body
exception described in [AARM 12.6(8.i–8.m)](https://www.adaic.org/resources/add_content/standards/05aarm/html/AA-12-6.html),
not a direct instantiation with an ordinary class-wide controlling actual.

The same roadmap explicitly lists generic-related Phase 8 work (prefixed and
implicit operator callable views, renaming-as-body completions and the broader
legality/staticness audit) and Phase 9 dependencies (concurrent implementations,
protected-operation actuals, named protected callbacks and task/protected
features). Existing limits on derivation before private completion and
composites still apply. Limited `in` objects remain diagnosed. The predefined Unchecked_Deallocation compatibility contract is
unchanged.
