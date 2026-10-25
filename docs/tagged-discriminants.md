# Tagged discriminants and private views (Phase 6f)

The single-parent tagged model supports discriminated roots and extensions,
including inherited discriminants, constrained parents and explicit new
extension discriminants. Discriminants have a logical declaration order separate
from the physical ancestor prefix. Named/positional constraints, defaults,
record and extension aggregates, dispatch, conversions and class-wide copies use
that distinction. A new discriminant part requires a constrained parent; tagged
discriminants cannot have defaults. Specified ancestor discriminants are reached
through an ancestor conversion rather than selectors of the new type.

Private extensions inherit visible ancestor components and operations while
keeping additional components private. They can precede the ancestor's full
view; their own completion follows the complete parent. Full record extensions
can derive from completed private views without exposing hidden parent fields.
Completion checks cover ancestry, explicit discriminant conformance, constrained
ancestors, limited keywords, taggedness and definiteness. Unknown-discriminant
partial views remain indefinite outside their package even when the full view
is definite, including through derivation: components and definite generic
formals reject them; objects and
allocators require initialization. Deferred constants remain legal.

These rules follow Ada 2005 [RM 3.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-4.html),
[3.7](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-7.html),
[4.3.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-3-1.html)
and [7.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-7-3.html).

## Layout and ownership

Runtime-sized tagged records capture size and field offsets during type
elaboration. The actual object's tag carries its layout, so dispatched bodies,
class-wide equality, ownership walkers and copies do not depend on a caller's
activation. Local type elaborations have separate tags/layouts in recursive or
concurrent activations. Extensions preserve the complete parent prefix, including
trailing Size padding. Construction uses the declared layout while an ancestor
tag is temporarily installed for controlled initialization and partial cleanup.

Dependent tagged array components have descriptors and separate owning arenas.
Ancestor-view assignment only finalizes/copies the ancestor's storage; extension
components retain their resources. Copy adjustment clones arrays and handles
allocation failure without adopting the source's resources. Controlled hooks and
component storage participate in the existing finalization chain.

Records containing still-private components reserve delayed layout tables.
Their tables and tags elaborate after component completions, in dependency
order, before objects may be declared. This also supports ordinary records and
arrays containing those records. Bounds retain declaration-time capture.

Static tagged first-subtype Size clauses can reserve aligned trailing storage;
too-small, nonadjacent size-changing and runtime-layout clauses are diagnosed.
A tagged subtype cannot change its type's shared tag layout. Tagged Alignment
and component representation clauses retain their existing restrictions.

## Limited current instances

A limited record's current instance is implicitly aliased in component defaults
and access-discriminant constraints. Its Access value uses the object under
construction and its actual accessibility, including allocator collection levels.
Inherited defaults retain the final object's identity. Named access defaults
perform dynamic accessibility checks; anonymous discriminants retain their
existing master checks. Ordinary nonlimited current instances are not aliased.

## Evidence and boundaries

`taggedviews`, `taggedruntime`, `taggeddependent`, `taggedcontrolled`,
`taggedinstances` and `privatedelayed` run whole/per-unit compilation,
deterministic IR checks and four concurrent workers. They exercise private
extensions, opaque initialized views, new/inherited discriminants, parent
padding, deep copies, ancestor assignment,
class-wide allocation/deallocation, controlled counts and current-instance
accessibility. `runtimecomponentlate` replaces the former delayed-layout
rejection with execution. `taggedviewerrors` and `privatecompletionerrors` pin
legality checks. The C array-storage test injects clone allocation failure and
checks runtime tag layout metadata.

Unchanged ACATS C3A0009 and C761012 are promoted from compile XFAIL to execution
PASS. Baseline C3A0007, C432001, C432004 and C760009 also pass unchanged.
C3A0013's private-extension and current-instance obstacles are removed; its
remaining compile XFAIL is `array (Object'Range)` declaration syntax (Phase 8).
C760011 now reaches the missing REPORT.Ident_Char adapter. B observations remain
ungraded.

Dependent arrays with controlled, task or protected elements, access-subtype
discriminant constraints and new discriminants on untagged derivations remain
diagnosed. Aggregates needing static selection of a parent variant through
new discriminant correspondences remain restricted. Indefinite generic formals
belong to Phase 6g; task/protected discriminants and broader freezing retain
their roadmap owners. Interfaces now follow the [interface contract](interfaces.md). These selected passes are not a full Ada conformance
claim. Sanitizers instrument the compiler or C runtime; QBE-generated loads and
stores are not instrumented.

Validation on Darwin arm64: 1,092/1,092 CTest cases; 17 focused compiler
ASan/UBSan cases; all seven new/promoted execution scenarios in whole and
per-unit modes with the instrumented C runtime; and runtime array-storage,
shared-table and dynamic-layout tests pass. The complete 106-case baseline was
reobserved and its changed diagnostics reviewed before recording.
