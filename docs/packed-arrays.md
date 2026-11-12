# Packed Boolean and signed integer arrays

The Phase 8 CD2A22A increment covers three connected requirements: Pack for
Boolean arrays, enumeration value Size independent of byte storage, and the
Unchecked_Conversion round trips used by the unchanged official Length_Check.
The existing [record bit-field contract](representation-clauses.md) remains in
force. Phase 12 extends the array machinery to signed integer component types
for unchanged CXC7002.

## Pack

`pragma Pack (T)` supports visible, unfrozen one-dimensional Boolean or signed
integer array first subtypes with nonaliased, nonshared components. Bounds may
be static or captured at runtime. Array subtypes inherit packing. Pack settles
the component's representation and requires storage-element alignment.
Unsupported targets, duplicate pragmas and pragmas after freezing are diagnosed.

Boolean storage uses one bit per component. Signed integer types use the minimum
1–64-bit width that represents their component subtype range: two's complement
when the lower bound is negative, otherwise an unsigned representation. An
[explicit component Size](https://www.adaic.org/resources/add_content/standards/05aarm/html/AA-13-3.html), including the nearest inherited specification for a
subtype, remains a minimum.
Components start at the least significant bit of the first byte and may cross
byte boundaries. Allocation rounds the logical bit count up to bytes; null arrays
retain their bounds and allocate through the usual arena. An unpacked outer
array of packed rows uses each row's rounded byte extent as its stride. Record
components and library/captured views preserve the same representation.

The array descriptor layout is unchanged. Negative stride values encode the
component width in bits; `-1` retains the existing `ADA_PACKED_BOOLEAN_STRIDE`
representation. Byte counts and offsets divide before scaling to avoid
intermediate overflow. Compiler and runtime agree on checked length-to-byte
rounding, offset checks, allocations, copies and owned array results. Component
addresses carry a bit offset; typed reads and masked writes preserve neighboring
components. Aggregates, assignment and comparison use the same layout. Boolean
logical operations retain their existing implementation. Typed loads sign-extend
negative integer representations; byte-wise masked stores preserve neighboring
components. Streams transfer logical scalar values rather than packed storage
bytes. Native Storage_IO snapshots validate signed components before committing
a read; invalid values raise Data_Error and leave the destination unchanged.
Snapshot writes clear padding bits. Scalar Ada calls retain their existing
value/copy-back ABI. Imported writable scalar components use a temporary and
normal-return copy-back.

Array type Size reports the logical bit count. Complete array-object Size
reports its rounded allocation extent; packed component Size is its selected bit
width. Scalar type Size is unaffected. Packed slices, concatenation, component
address/access attributes, generic formal components, modular/ordinary enumeration Pack targets,
multidimensional Pack, conversions between different packed representations and
nondefault bit order remain explicitly unsupported. Matching packed layouts
support the existing array conversions. This increment adds no follow-on roadmap
items.

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

## Phase 8 validation

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

## CXC7002 validation

Unchanged CXC7002 checks packed signed-integer task attributes, independent initial
values, references, replacement and task identity/lifetime behavior. It runs under
strict REPORT checking in both whole and separate-unit modes.

`packedsigned` covers 1-, 4-, 5- and 64-bit signed components and nonnegative
integer subtypes, explicit/inherited component Size minima, negative/runtime/null
bounds, nested arrays, record defaults,
independent returned copies, allocation/deallocation, matching-layout conversions,
scalar copy-back and
exception paths, renaming Size, cross-byte scalar streams and native snapshots with corrupted-input
validation and destination preservation. Four callers each
repeat the checks with independent state. `packedsignederrors` pins unsupported
representations, slices/concatenation, component addresses and signed-array
Unchecked_Conversion. The latter remains outside the supported conversion subset.

`runtime.array_descriptors` checks all widths 1–64 at every starting bit offset
against a bit-by-bit preservation oracle, with surrounding canaries, in four
threads. It also covers signed-64-bit byte-count/offset boundaries and overflow.
Darwin arm64 validation: all 2,182 Release checks pass (312 ACATS), including
all 313 legality fixtures; all 29 focused ASan/UBSan checks pass. The full
161-case baseline recheck is stable, with only CXC7002 promoted from the previous
snapshot: 134 C passes, one C rejection and 26 raw B rejections. All 26 independent
B grades pass. Source hashes, the 161-row/22-area/26-grade reconciliation and
`git diff --check` pass. QBE is unchanged.
