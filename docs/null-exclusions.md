# Null exclusions

Phase 6d supports `not null` in named access type definitions and derived
parents, subtypes, objects, record/array components, discriminants, parameters,
results, access-to-subprogram profiles and renamings. Anonymous object,
constant, ordinary callback and protected callback definitions use the same
checks. Named and anonymous access results support extended returns.

The exclusion belongs to the nominal subtype; it does not remove the null
value from the access type. Comparisons with null remain legal. Subtypes and
derivations retain their parent's exclusion. An explicit exclusion on a named
subtype that already excludes null is illegal, as is an exclusion on a
non-access subtype (Ada 2005 RM 3.10(14.1/2)).

Default initialization still supplies null and therefore raises
`Constraint_Error` for an excluded subtype. Initializers, assignments,
aggregates, conversions, qualifications, parameter passing, function returns
and copy-back check the destination subtype before publishing the value.
An `out` actual is copied in without a null check; assignments inside the
callee and copy-back enforce their respective nominal subtypes. A failed
assignment leaves the destination unchanged. Component initialization uses
the existing partial-construction cleanup paths. No runtime ABI change or
new shared state is required.

Null exclusions participate in static subtype matching, body conformance,
overriding and callback profile matching. They do not distinguish overloads.
Controlling access parameters retain their implicit exclusion; full body
conformance also checks whether the exclusion was written explicitly.
An explicit null-excluding renaming requires an already null-excluding view;
a renaming without an explicit exclusion inherits the renamed view's subtype.

Generic formal subprograms with explicit exclusions require matching actual
parameters/results to exclude null. A generic formal subprogram forwarded as
an actual within a generic body must have an explicit exclusion for the
corresponding promise.
Instance wrappers preserve the actual's exclusion even when the formal's
named subtype differs. Generic access objects and formal access types remain
unsupported until 6g, with diagnostics; their null-exclusion scenarios are
not claimed as implemented. Existing limited/tagged/discriminant restrictions
also remain in force.

`nullexclusions`, `nullexclusionerrors`, `nullexclusiongenericerrors` and
`nullexclusionsyntaxerrors` and `nullexclusiontasking` cover successful values, default initialization,
checked conversions/qualifications, direct/anonymous calls and results,
extended returns, components, callbacks, renamings, generic matching and
failed copy-back. Executable cases run whole/separate compilation and
determinism checks; four workers exercise concurrent failures.

Unchanged ACATS C3A0030 passes (nullable and excluded access discriminants).
C3A0020 is a pinned compile XFAIL for generic access objects (6g); C3A0025
also needs general limited records (6f). These XFAILs do not claim execution
of their runtime scenarios. B-test protocol remains a Phase 8 harness gap.

Normative references: [RM 3.10](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-10.html),
[RM 6.3.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-6-3-1.html),
[RM 12.6](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-12-6.html).
