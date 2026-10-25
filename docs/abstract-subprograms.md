# Abstract subprograms

AIDA implements Ada 2005 abstract subprogram declarations for the supported
profiles and tagged type forms ([RM 3.9.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-9-3.html)).
They require no body and cannot have a body or renaming-as-body completion.
Abstractness follows inherited operations, including equality and its implicit
inequality. Concrete descendants must supply nonabstract overrides before
freezing; abstract extensions can retain or redeclare inherited obligations.
A non-null abstract extension's inherited controlling-result functions become
abstract even when their ancestors have bodies.

Abstract primitives require abstract tagged types. Private abstract primitives
of visible types must override visible inherited abstract operations. Abstract
partial views may have concrete full views; clients retain the abstract partial
view, while internal code uses the full view. Class-wide types are concrete.
Objects, allocators, aggregates, components, assignment targets and generic
formal `in` objects cannot have an abstract specific type. Functions returning
an abstract specific type must themselves be abstract.

Calls to abstract operations must dispatch. Analysis waits until enclosing
calls and class-wide assignments have supplied tags to tag-indeterminate
results. Statically bound inherited operations forward their descendant tag
to factory defaults while invoking the parent body. This includes operators,
controlling access parameters, qualified names and defaulted controlling operands. A primitive renaming can dispatch;
a nonprimitive renaming cannot ([RM 3.9.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-9-2.html)).
Abstract actual types require abstract generic formal types. Abstract
subprograms cannot be generic actuals for ordinary formal subprograms,
or prefixes of Access, Unchecked_Access or Address attributes.

Abstract dispatch slots contain null code pointers; abstract tags have no
class-wide equality thunk. No abstract body or inherited callback entry is
emitted. Concrete overrides reuse the inherited slots and normal static links.
Record aggregates of supported record extensions can supply inherited fields
directly ([RM 4.3.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-3-1.html));
abstract types remain forbidden as aggregate result types, but can be ancestor
subtype marks in concrete extension aggregates.

## Boundaries and validation

Interfaces, private extensions and discriminated tagged types retain their
later 6f owners. Dispatching functions with controlling anonymous access
results now receive an explicit diagnostic; their dispatch and inherited
required-overriding rules remain in the next 6f legality item. Generic abstract
formal types/subprograms remain 6g. These restrictions are not silent acceptance.

`abstractsubprograms` and `abstractinheritance` cover whole/per-unit/deterministic
compilation and four workers, including defaults, factories, operators,
equality, access parameters, private views and multiple inheritance levels.
`abstract*errors` cover declaration, completion, inheritance and use legality.
The unchanged ACATS C460006 passes tagged view conversion, renaming, assignment
and dispatch checks and joins the strict baseline selection. B840001 remains
an ungraded observation after three legal record-extension aggregates become
accepted.

Validation: full CTest suite 1,050/1,050; focused compiler ASan/UBSan checks
and unchanged C460006/B840001 sanitizer probes pass. The complete 106-case
observational baseline matches its updated snapshot.
