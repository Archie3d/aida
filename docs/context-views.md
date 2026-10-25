# Limited and private context views

AIDA implements Ada 2005 `limited with`, `private with` and `limited private
with` for its supported library/type subset. The governing rules are
[RM 10.1.1–10.1.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-10-1-2.html),
[3.10.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-10-1.html)
and [8.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-8-3.html).

## Visibility and identity

Limited views contain incomplete views of visible type declarations and limited
views of nested nongeneric packages. They have no imported context, objects,
subprograms, subtype declarations or generic instances. Tagged declarations
produce tagged incomplete views, including class-wide access designations.
Their identity is shared with the full types, but their visible representation
stays incomplete regardless of analysis or source-argument order. Package
renamings also denote the limited view where that view is visible.

Named/anonymous object access definitions, unconstrained subtype aliases and
tagged incomplete formal profiles are supported. Objects, array/record
components, allocators, derivations, conversions, qualification, generic actual
types and representation attributes require complete views. Limited package
views cannot be used. Renamed limited packages can be named only in the
renaming's immediate scope or a context that withs their defining library
package; ordinary views can then make those aliases complete. Limited context clauses cannot name generic
units, instances, renamings or subprograms, appear on bodies/renamings, name
their own library declaration, or overlap a nonlimited context clause for the
same package, including inherited clauses.

An access dereference or exported incomplete subtype alias becomes complete
where the completion or an ordinary `with` of its defining package is visible.
An ordinary `with` in a package body can therefore implement operations on
pointers introduced through a limited view in the specification. A client that
withs only the pointer-defining package still sees an incomplete designation.
Record component expressions retain the view used for resolution, so checks
and field attributes use the same layout.

Private context names become usable in private parts, bodies and private
descendants, including nested private parts and public descendants' bodies.
They remain hidden in visible parts, context use clauses and library subprogram
body profiles. Private child declarations and their context restrictions are
checked. `limited private with` can name a private child from its parent spec;
the child's implicit dependence on its parent does not form a semantic cycle.

## Completion and elaboration

Local incomplete declarations support `is tagged`, known discriminants and
unknown `(<>)` discriminants. Known completion discriminants must conform;
tagged incomplete declarations require tagged completions. Visible-part
incompletes require completions in that visible part. Private-part incompletes
may be completed in the body unless primitive declarations require completion
in the private part. Intervening bodies freeze ordinary incomplete types.
Incomplete declarations and aliases emit no initialization; completed types
supply their layouts, bounds, collections and tags at the full declaration.

Limited edges cause discovery and partition inclusion, but impose neither full
semantic ordering nor elaboration ordering. Manifests record `limited` edges
separately from `requires` edges. The binder verifies their target membership
without ordering them. Ordinary/private nonlimited edges retain their normal
ordering. Compilation records retain source/specification digests for limited
dependencies: changed specs invalidate clients; ordinary dependency bodies do
not. Cycles containing a limited edge can compile, while ordinary semantic
cycles remain errors.

## Evidence and remaining type limits

`limitedwith`, `privatewith` and `taggedincomplete` run in whole-program,
per-unit and deterministic-IR modes. Coverage includes mutual packages,
scalar/record/class-wide designations, anonymous results, completed profiles,
local and private-body completions, known constraints, runtime-bounded array
aliases, private children, nested views, allocation/deallocation and four tasks.
`semantics.context_views` checks legality, source-order independence, exported
incomplete aliases and renamed views. `loading.context_dependencies` verifies
metadata, binder membership, mixed/ordinary cycles and incremental spec/body
invalidation. Compiler AddressSanitizer checks cover the new paths as well.

Private extensions, tagged discriminants and unknown-discriminant private views
now follow the [Phase 6f contract](tagged-discriminants.md); general limited
records landed with 6e returns. Unknown discriminants on full definitions remain
illegal. Subprogram/package
[subunits](subunits.md) are now implemented.
[Object `Unchecked_Access`](unchecked-access.md) subsequently
landed as an explicit follow-up. Unchanged ACATS CA20003 passes partition
inclusion and CA110232 passes mutual references. CA12002 also passes with
separate subunits. CA120012 now also passes unchanged after general
limited-record definitions were pulled forward into 6e.
