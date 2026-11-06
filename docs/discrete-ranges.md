# Discrete ranges

Array definitions accept subtype marks, explicit subtype ranges, and bare
ranges whose bounds are simple expressions, including calls and qualified
expressions. Scalar range constraints, array index constraints, slices and
array aggregate choices accept range attributes. Explicit subtype ranges in
index constraints, slices and aggregate choices retain the named subtype for
compatibility checks.

`Prefix'Range` and `Prefix'Range (Dimension)` use the existing paired First/Last
representation. The selected dimension supplies the index type, including
an enumeration type, rather than defaulting to Integer. Dimensions must be
static and within the array rank. A call in the prefix is evaluated once for
the pair. Declaration-time bounds are captured using the existing array or
scalar bounds descriptors; later objects reuse those bounds. Record component
constraints retain declaration-time captures and per-object discriminants.

Non-null explicit ranges must fit both the named subtype and the array index
subtype. Incompatible bounds raise Constraint_Error when the declaration,
slice or aggregate is evaluated. This includes static incompatibilities at
declaration elaboration. Null ranges retain their endpoints and do not require
either endpoint to belong to the subtype. Conversions inside bound expressions
still perform their own checks.

Named array aggregate choices without `others` supply their own bounds. Shape
checks occur at assignment/conversion boundaries, while equality can compare
arrays with different lengths, including a null-range aggregate.

`discreteranges` checks function and parameterless-call prefixes, scalar and
array constraints, enum index identity, named dimensions, runtime capture,
record components, slices, null ranges and exception timing. It runs in the
main task and four independent workers, with whole-program, separate-compilation
and deterministic IR checks. `discreterangeerrors` checks wrong index types,
nondiscrete subtype marks and invalid/nonstatic dimensions.

Unchanged C35003A, C36104A/B, C36180A, C37002A, C41207A, C43204A and C3A2003
execute as PASS. C3A0013 remains a control for the earlier Object'Range array
definition support. Existing restrictions on discriminant expressions,
array storage and the surrounding generic/representation features still apply;
this increment does not add access-subtype record discriminant constraints.
