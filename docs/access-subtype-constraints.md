# Access-subtype designated constraints

Named access subtypes accept discriminant constraints on an unconstrained
designated record subtype. The access subtype retains its access type,
collection and accessibility identity; its designated subtype records the
constraint. Positional, named and grouped associations reuse the existing
record constraint analysis, including type checks and generic contract binding.
See Ada 2005 RM [3.7.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-7-1.html)
and [4.6](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-6.html).

Constraint expressions are captured when the subtype, object, component or
array-component declaration elaborates. Each grouped selector evaluates its
expression once per associated discriminant. Subtype aliases and derived
access types retain the capture. Library captures are exported and reused
across compilation units. Expressions that depend on a containing record's
discriminants remain per-object; other expressions in the same constraint
are captured at the component declaration.

A nullable access subtype accepts null without reading designated storage.
A non-null value must designate an object whose discriminants match the
constraint. Initialization, assignment, explicit conversion, input parameters,
function results and scalar parameter copy-back use the same check. A failed
check raises Constraint_Error before storing into the destination. An access
component's containing object travels with its address, so assignment and
copy-back use that object's discriminants. Null exclusions retain their
existing checks. Discriminant checks retain their existing suppression policy.

Conversions between related access types can require a runtime designated
constraint check even when their constrained subtypes do not statically match.
Conversions between unrelated general access types retain the existing
legality, representation and accessibility rules. Static subtype conformance
compares designated constraints rather than treating all subtypes of an access
type as matching.

A designated subtype cannot be constrained again. Unknown-discriminant views,
missing/duplicate/unknown selectors, range values and incompatible discriminant
types are diagnosed. Access constraints also reject constrained partial views
and general access to records with defaulted discriminants, with the generic
body presumptions required by RM 3.7.1(7/2). Pool-specific access to records with
defaulted discriminants remains supported. Phase 12 extends this contract to
array constraints below and [controlled dependent arrays](controlled-dependent-arrays.md).

`accesssubtypes` and `access_subtype_checks` cover library captures, aliases,
derived and constant access, private designated records completed across units,
grouped constraints, per-object components, nulls, exception timing and parameter
copy-back. The main task and four workers run the checks with independent local
state. Whole-program, separate-compilation and deterministic IR checks pass.
`accesssubtypeerrors` pins legality and subtype-conformance diagnostics.
Unchanged C37003A and C37006A execute as PASS; the complete 129-case baseline
records 92 executable passes, ten compile rejections, one support blocker and
26 ungraded B observations. No runtime ABI or external QBE changes are needed.

Validation: 1,743/1,743 Release CTest checks and 9/9 focused ASan/UBSan checks
pass, including the new fixtures, both upstream cases and existing record/access
controls. The pinned upstream sources remain unchanged.

## Array constraints and deferred constants (Phase 12)

An index constraint on an access subtype constrains its designated array.
Array and access-to-array constraints share rank, index-type, range and capture
analysis. The access type retains its identity and collection; constraining an
already constrained designated subtype is rejected. Named discriminant-style
associations and incorrect ranks remain errors.

Named bounds are captured at declaration, while component constraints referring
to record discriminants are checked against each enclosing object. Null values
are accepted by nullable constrained access subtypes. Non-null values must have
the designated bounds on every dimension, with no sliding; initialization,
assignment, allocator results, conversion, parameters and function returns use
the same bounds check. Allocators still convert/slide an initializer into the
access type's first designated subtype; an additional access-subtype constraint
is checked on the resulting pointer. Statically constrained raw address views
keep their known bounds without requiring an allocation registration.
Dereferencing a per-object constrained access component uses its actual
registered array view.

Deferred constants with unconstrained array or discriminated record subtypes
are accepted in a package's visible part; their private completions supply the
value and bounds/discriminants. They are not treated as immediate uninitialized
objects. Ordinary indefinite object declarations retain their checks.

`accessarrayconstraints` checks captured and per-object bounds, nulls, mismatched
lower bounds and lengths, allocators, parameters/results and deferred constants
in whole-program/per-unit execution with deterministic IR. `accessarrayconstrainterrors`
checks repeated constraints, rank errors and named associations. Unchanged
C761010 and CC3605A now execute successfully.
