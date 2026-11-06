# Packed Boolean arrays and bit-sized enumerations

The Phase 8 CD2A22A increment covers three connected requirements: Pack for
Boolean arrays, enumeration value Size independent of byte storage, and the
Unchecked_Conversion round trips used by the unchanged official Length_Check.
The existing [record bit-field contract](representation-clauses.md) remains in
force.

## Pack

`pragma Pack (T)` supports visible, unfrozen one-dimensional Boolean array first
subtypes with nonaliased, nonshared components. Bounds may be static or captured
at runtime. Array subtypes inherit packing. Pack settles the component's
representation and requires storage-element alignment. Unsupported targets,
duplicate pragmas and pragmas after freezing are diagnosed.

Storage uses one bit per component, starting with the least significant bit of
the first byte. Allocation rounds the logical length up to bytes; null arrays
retain their bounds and allocate through the usual arena. An unpacked outer
array of packed rows uses each row's rounded byte extent as its stride. Record
components and library/captured views preserve the same representation.

The array descriptor layout is unchanged. Its previously unused stride value
`ADA_PACKED_BOOLEAN_STRIDE` distinguishes a packed Boolean dimension from byte
strides. Compiler and runtime agree on checked length-to-byte rounding, offset
checks, allocations, copies and owned array results. Component addresses carry
a bit offset; typed reads and masked writes preserve neighboring components.
Aggregates, assignment, comparison and Boolean logical operations use the same
layout. Scalar Ada calls retain their existing value/copy-back ABI. Imported
writable scalar components use a temporary and normal-return copy-back.

Type Size reports the logical bit count. Complete array-object Size reports its
rounded allocation extent; packed Boolean component Size is one bit. Scalar
Boolean type Size is unaffected. Packed slices, concatenation, component
address/access attributes, broader Pack targets and nondefault bit order remain
explicitly unsupported. This increment adds no follow-on roadmap items.

## Enumeration Size

Enumeration Size accepts 1 through 32 bits when every stored representation code
fits. Storage rounds to the existing 1/2/4-byte scalar formats; the requested
value Size is retained independently and inherited by subtype aliases. Both
orders of Size and enumeration mapping clauses are checked. Insufficient widths,
invalid codes and late representation clauses are diagnosed.

Type/subtype Size reports the selected value Size. Stand-alone objects and
ordinary array/record components retain byte storage and object Size at least
as large as that value Size. Arithmetic, comparison, Pos/Val, Succ/Pred, Image,
Value and parameter/result passing continue to use enumeration positions.
Typed memory boundaries encode/decode mapped codes as before.

## Unchecked_Conversion

`Ada.Unchecked_Conversion` and its J.1 root compatibility view have the standard
indefinite limited-private formal profile and intrinsic callable behavior.
Supported actuals are discrete scalars and packed Boolean arrays. A packed
array target must have constrained bounds, which may be captured at runtime;
an unconstrained packed source takes its bounds from the argument. Other
categories are diagnosed at instantiation. This is a documented restriction
under [RM 13.9](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-9.html).

Calls evaluate the argument once and copy its representation into independent
target storage without converting its numeric value. Conversion size is the
scalar subtype's value Size or packed array's logical bit count; descriptors
and padding are excluded. Equal sizes are required at runtime; mismatch raises
Program_Error before copying. Non-byte-aligned copies clear destination padding.
Signed enumeration representations are sign-extended from their selected width
before decoding. Invalid scalar results raise Constraint_Error. Renamings and
generic callable binding retain the intrinsic implementation; Access is rejected.

## Validation

`packedstorage` exercises runtime lengths, nested rows, negative/null bounds,
library functions, record components, aggregates/copies, packed logical
operations, scalar component copy-back and exception paths, Size, mapped codes,
intrinsic renaming, unconstrained sources, invalid values and unequal sizes.
`lengthcheckstorage` executes the unchanged official Length_Check body for
three-bit enums, signed codes, 9/24-bit enums, Integer and modular Byte. Each
program uses four workers repeatedly creating independent local state. Both
run whole, separately and deterministically, with the official helper and
REPORT support passed explicitly to each compilation.

`packederrors`, `bitsizeerrors` and `uncheckedconversionerrors` pin legality and
subset diagnostics. `runtime.array_descriptors` verifies packed byte counts,
large/null lengths, offsets, owned results, bit-copy masks/canaries and mismatch
failure in independent threads. Unchanged CD2A22A executes its full REPORT
protocol as PASS with its unchanged official helper dependency; the local
Length_Check calls also exercise that dependency's body.

The full Release suite passes 1,774/1,774 checks. All 12 focused checks pass with
ASan/UBSan compiler/runtime instrumentation, including whole/separate execution
of both local scenarios and the full unchanged CD2A22A. QBE-generated Ada memory
accesses remain uninstrumented; LeakSanitizer is disabled on this host. The full
129-case backfill refresh retains 92 PASS, 10 COMPILE_REJECTED, one HARNESS_BLOCKED
and 26 ungraded B rejections; only its compiler hash changes. The original
CD2A22A and Length_Check SHA-256 hashes match the pinned ACATS inventory.
