# Ada.Storage_IO (Ada 2005)

The source generic provides the [A.9 profile](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-A-9.html):
the definite nonlimited private formal Element_Type, Preelaborate, Buffer_Size,
Buffer_Type, Read, Write, and Data_Error renamed from Ada.IO_Exceptions.
Buffer_Type is a subtype of System.Storage_Elements.Storage_Array with bounds
1 .. Buffer_Size. Instances share that array type; Buffer_Size is specific to
each actual subtype. Public Read/Write are ordinary Ada procedures, supporting
renaming, generic actuals and subprogram Access.

## Buffer representation

Buffer_Size uses Element_Type'Max_Size_In_Storage_Elements: the native storage
extent, including record/array padding and explicit Size reservations. The
compiler now supports this attribute on complete definite subtype prefixes
without dependent storage. Runtime-constrained subtypes use their captured
bounds; calls do not reevaluate the bounds expressions. Unconstrained prefixes,
object prefixes and arguments are diagnosed.

The format follows native scalar storage, record offsets and array strides.
Write first clears aligned staging storage, then stores logical components,
leaving padding and inactive variant storage zero. Enumerations retain their
representation codes. Represented fields and packed Boolean arrays retain bit
placements, with unused Boolean tail bits cleared. Null arrays require zero
bytes. Tagged object headers become zero placeholders; Read reconstructs the
instance's specific tag instead of importing a tag pointer.

Read stages bytes in aligned storage before loading scalars. Slices and non-1
actual bounds use ordinary array sliding; staging protects overlapping views.
Wrong buffer lengths raise Constraint_Error at the public call boundary.
Scalar subtype violations, invalid enumeration codes, null-exclusion failures
and nonzero tag placeholders raise Data_Error during decoding. Floating values
use the compiler's finite scalar model. All components are validated before
assignment changes Item, so invalid scalar/composite data leave it unchanged.
Validation remains enabled under Suppress. Read ignores unused padding and
Boolean tail bits.

This is an implementation representation, not a portable file/network format.
Layout, byte order, scalar representation and subtype constraints must agree.
Compatible buffers can be saved through Direct_IO, including records containing
runtime-sized inline arrays. User-defined stream attributes are independent.

## Assignment, ownership and supported actuals

Read uses ordinary Ada assignment, preserving controlled Finalize/Adjust and
object/collection rules. It first acquires an adjusted, registered temporary for
controlled parts, protecting resources when Item is also the original object.
Successful temporary parts are finalized on normal and exceptional exits.
Adjustment failures retain the existing Program_Error behavior; a failed Adjust
hook must undo its own partial resource acquisition.

Write does not acquire ownership, call Adjust, or register represented objects.
The byte array has no controlled lifetime. Access values and references inside
user-controlled types are shallow, borrowed references: their resources must
still be alive whenever Read uses the snapshot. Successful Read acquires a
controlled copy through its Adjust hooks. Keeping bytes does not keep resources
alive or make foreign/stack storage eligible for deallocation. Arbitrary pointer
bytes cannot be validated as live references; such buffers must contain valid
native access representations. Durable resources need user-defined serialization.

Supported actuals include discrete/real/fixed scalars, access values, completed
private types, plain and controlled records, fixed-layout variants, static and
runtime-constrained arrays, multidimensional arrays and inline nested arrays.
Runtime layouts use captured sizes and field offsets. Formal actuals are checked
again at concrete instantiation.

Discriminant-dependent owned component storage, class-wide values, task/protected
parts, file handles and exception-occurrence resources currently produce
diagnostics. Runtime ownership records are never copied into unrestricted bytes.
Limited and indefinite actuals fail the generic's ordinary private formal
contract. These are explicit implementation gaps, not A.9 permissions to
restrict instantiation; unrestricted A.9 conformance is not claimed.

## Validation

`storageio` covers scalar/fixed/modular/represented-enumeration values, private
and padded records, represented fields, variants, strings/matrices/packed arrays,
null and runtime-sized nested arrays, large buffers, Read Access, renaming and
nested generics. Side-effecting arguments run once. Wrong extents, invalid data,
controlled resource self-assignment, failed Adjust cleanup and four workers have
focused controls. Whole/separate compilation and deterministic IR are checked.
`storageioerrors` and `storageioprofileerrors` cover generic legality,
representation restrictions, callable profiles and malformed internal imports.

Unchanged ACATS CXA9001 and CXA9002 pass their complete REPORT scenarios:
runtime-sized nested record/array buffers persisted through Direct_IO, and
tagged/extended record buffers. No upstream sources are edited or reduced. The
[manifest](../tests/acats/storage_io/manifest.json) pins original source paths,
SHA-256 and Git-blob hashes at ACATS revision
`2785c30c1f8577d97ac7b3043882f6818781e9c5`; hash checks and both executions are
registered separately in the regular suite.

Release validation passes all 1,826 tests. All 18 focused ASan/UBSan checks pass,
including malformed-input recovery,
controlled assignment, storage-size regressions and address conversions. The
original 145-case ACATS backfill outcomes remain unchanged.
