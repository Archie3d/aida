# Controlled components in variant records

Variant records support ordinary and Limited_Controlled components, including
nested variant parts, records and declaration-sized arrays containing controlled
parts. Initialization, adjustment and finalization visit only components of the
active alternative, including every enclosing variant selector. Existing
limited-copy restrictions apply to the whole type, even when the current arm
has no limited component.

Unconstrained assignment checks the target constraint before finalization,
constructs an adjusted snapshot, finalizes the old active components, copies
and adjusts the new components. Self-assignment preserves the object.
Constrained targets reject a discriminant change with Constraint_Error.
Overlaid components can have different controlled types and finalizers.

Cleanup registrations are reserved for all potential controlled parts before
construction, without reading uninitialized discriminants. Only successfully
initialized or adjusted components become active. Reservations at an overlaid
address reuse the current registration with the selected component's finalizer;
unused reservations remain inactive. This bounds registration storage across
repeated assignments. Partial initialization and adjustment failures clean up
completed parts, and failed finalization does not prevent cleanup of other
parts. Existing Program_Error rules for failed adjustment/finalization apply.

Local, temporary, library and collection ownership use the existing lifetime
chains. Allocators, deallocation, function results, extended limited returns,
array elements and class-wide copies of tagged containers use the same rules.
The concrete tag's component walker has a reservation mode that includes all
alternatives; adjustment and finalization modes select active components.
Record extended returns capture their actual discriminant constraints inside
the function before constructing the result.

Controlled arms can coexist with discriminant-dependent ordinary array
storage. Dependent arrays of controlled/task/protected elements remain
unsupported, as do task/protected variant components, new discriminants on
derived tagged types, managed tagged dependent storage and general limited
records outside the existing controlled subset. These constructs retain
explicit diagnostics. Byte-addressable [record representation clauses](representation-clauses.md)
now support these controlled components.

`controlledvariants` checks nested/null arms, distinct overlaid finalizers,
mutable/constrained/self assignment, default and boxed construction, copies,
function results, allocators, deallocation, partial failure and recovery,
class-wide assignment, dependent payloads and arrays of mutable records.
`controlledvariantlimited` checks constrained extended returns, limited
construction and exceptional scope exit. Both run whole/separate compilation
and deterministic IR checks. `runtime.array_storage` checks overlaid cleanup
registrations and verifies all allocation and registry storage is released.

Unchanged ACATS C760007 passes controlled adjustment/copy requirements.
C761012, covering anonymous variant-object lifetime after a failed component
selection, is retained as an exact compile XFAIL because it requires new
discriminants on a derived tagged type. It is not an executable pass or a
controlled-variant conformance claim.
