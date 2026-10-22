# Runtime scalar constraints

Phase 6b extends the existing scalar-bound representation. The target rules are
[Ada 2005 RM 3.2.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-2-2.html),
[3.3.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-3-1.html),
[3.5](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-5.html),
[3.8](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-8.html)
and [13.13.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-13-2.html).

## Elaboration and ownership

Named and anonymous scalar constraints can capture runtime bounds in local and
library declarations. This includes object declarations, scalar record/array
components, access designated subtype indications and derived-type constraints.
Integer, modular, enumeration, binary32/64 floating and ordinary/decimal fixed
ranges share the same elaboration/check machinery. Type-definition bounds and
accuracy constraints that require static expressions remain static.

Semantic analysis records the constraint's elaboration plan on its owning
declaration. Each newly constrained dynamic subtype has a bound symbol immediately,
so later static folding cannot use its placeholder limits. Aliases retain that
symbol without reevaluating the original expressions. Bounds are captured once;
changing the variables that supplied them does not change the subtype.

Grouped object declarations have independent nominal subtypes, elaborated in
identifier order before each corresponding initializer. Grouped record components
also have independent constraint plans, elaborated at the record declaration,
not each time a record object is created. Component defaults still run when the
object is initialized. A deferred scalar constant's completion must statically
match its visible nominal subtype; two independently elaborated anonymous dynamic
constraints do not match merely because they refer to the same bound variable.

Bounds occupy two eight-byte slots. Locals use the existing owning activation
and lexical links; recursion and tasks therefore have independent storage.
Library constraints use exported, aligned data with names derived from semantic
scope and source location. Whole and separately compiled clients reference the
same slots. Library data is established during normal elaboration and is then
read by dependent tasks. Generic instances capture their formal-object bounds
through the same path. No new mutable C runtime global is introduced.

Floating bounds use their declared QBE class (`s` or `d`) for storage, loads and
comparisons. Their First, Last, Range and membership operations use captured
values. Static real constraints also check compatibility during elaboration.
For all scalar categories, each endpoint is checked against the base
representation; a nonnull range additionally checks its parent subtype. Null
ranges retain their endpoints and contain no values. Decimal digits limits are
checked independently, including for null explicit ranges.

Assignments, conversions, qualifications, parameters, copy-back, returns and
imported results retain the existing shared range checks. Failure occurs before
a target value is overwritten. Initialized scalar allocators additionally check
the access type's designated subtype and use that subtype's storage size.
An explicit scalar allocator constraint is illegal, rather than a new anonymous
constraint context (RM 4.8(4)); it receives a diagnostic.

## Width and streams

Runtime discrete Width is zero for a null subtype. Signed/modular widths use the
longest endpoint image, including the sign/blank column, without overflowing on
INT64_MIN or the maximum 64-bit modular value. Enumeration widths scan literal
names between the captured positional bounds; representation codes do not change
the result. Character width follows the existing three-byte image model. Width
requires a discrete subtype prefix with no arguments. Wide images/widths and the
broader character model remain Phase 7 work.

Scalar streams keep their existing native byte representation; no bound
metadata is added to the stream. Input checks the prefix's current captured
subtype. Read checks the actual object's nominal subtype through temporary
storage and leaves the object unchanged on failure; the stream still advances.
Read's prefix constraint does not narrow a wider actual. Write/Output accept
base-type values as before. These rules also cover floating, fixed and
represented enumeration scalars. Composite/user-defined streaming remains the
separate library roadmap item.

## Validation and boundaries

`scalardynamic` covers library snapshots, grouped constraint order, scalar
components, derived types, recursive real subtypes, null ranges, generic bounds,
deferred constants, fixed/decimal anonymous constraints, failed conversions,
qualifications, returns and allocator checks. Width cases include unsigned
2**64-1, signed INT64_MIN, Boolean, Character and an enum with a longest interior
literal. `scalarstreams` covers successful and failing integer/real/fixed/enum
reads, represented enums and unchanged targets. `scalartasking` reads library
and task-local constraints from four tasks. All three run whole/separate and
determinism checks; named local scalar and existing numeric regressions remain.
`scalardynamicerrors` pins type, staticness, Width, allocator and deferred-constant
legality. `runtime.scalar_bounds` checks the C helpers at their integer endpoints.

The full suite has 692 passing expectations, including 19 selected ACATS cases.
Unchanged upstream C35507A adds executable Character Width coverage; it does not
prove all dynamic subtype contexts. ASan/UBSan covers the scalar helper and
allocation-storage tests; the new Ada programs run in both compilation modes
with ASan/UBSan and ThreadSanitizer runtimes. Generated Ada memory accesses are
not instrumented, so runtime instrumentation is not a complete Ada race check.

Library arrays with runtime bounds, runtime-constrained array components and
allocators, and box aggregates remain the next Phase 6b items. Discriminant-
dependent components remain Phase 6c. General access/accessibility, legacy delta
constraints, full freezing, portable stream representations and wide character
facilities remain separately tracked. Existing unsupported constructs continue
to receive diagnostics; no general dynamic layout or new parameter-profile
syntax is introduced here.
