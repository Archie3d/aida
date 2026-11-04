# Representation clauses

Ada 2005 RM 13.1 and [13.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-3.html)
and [13.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-4.html)
are the baseline. Phase 5 implements byte-addressable Size, alignment and
enumeration mapping. Phase 6c adds byte-addressable record component clauses,
including variants and bounded discriminant-dependent arrays. Phase 8 adds
packed and unaligned discrete slots under
[RM 13.5.1–13.5.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-5-1.html).

## Alignment subset

`for T'Alignment use N;` supports complete, visible, untagged first subtypes
without task or protected parts. N is a static integer: 0, 1, 2, 4 or 8 storage
elements (one element is eight bits). Zero removes the subtype's alignment
promise; allocation still uses the natural alignment. Positive requests cannot
weaken natural alignment. Values above eight, non-power-of-two values, duplicate
clauses and unsupported targets produce diagnostics.

Alignment clauses for untagged records follow the record layout freezing
rules below. Other Alignment clauses still require an adjacent group before
layout use. Alignment clauses on named subtypes or individual objects remain
unsupported, as do tagged/task/protected Alignment clauses.

The layout helper propagates alignment to subtypes and derived types, array
components, record field offsets, local/captured/global objects and allocations.
Records gain tail padding to their alignment. Scalar widths remain unchanged;
requests needing scalar or array stride padding are rejected. An unconstrained
array accepts an increased alignment only when each element's size is divisible
by it. Requests after a Size clause use the selected width; a subsequent Size
clause cannot shrink storage below its specified alignment. Alignment cannot
invalidate an earlier confirming composite Size clause.

Subtype and object `'Alignment` queries accept no arguments. Object queries
return the alignment guaranteed by their subtype's allocation and evaluate the
prefix once, including function calls. Class-wide and hidden representations
are diagnosed. A zero subtype alignment is reported as zero; objects still
report their natural guaranteed alignment.

## Enumeration subset

`for Color use (Red => -7, Green => 11, Blue => 250);` specifies stored
codes independently of positions 0, 1 and 2. Positional aggregates, named
associations in any order, static enumeration choices and a final `others`
are supported. Every literal must be covered exactly once. Codes must be
static integers, distinct and increasing in declaration order (RM 13.4(6)).
Multiple literals selected by a range, choice list or `others` would receive
the same code, so such associations are legal only when they select one literal.
Character enumeration literals are case sensitive and retain their declared
positions, spelling and case in Image/Value.

The implementation supports signed 32-bit codes and chooses one, two or four
bytes of storage, using unsigned byte/halfword storage for nonnegative codes.
An adjacent Size clause can select 8, 16 or 32 bits if all codes fit; either
clause order is accepted. Alignment promises must still fit the chosen storage.
A subtype inherits its type's mapping; a derived type inherits the mapping
and can replace it with an adjacent clause of its own. Corresponding subtypes
in inherited primitive profiles follow the derived storage width and alignment.

Clauses require a complete, visible first subtype and an adjacent group of
representation items, before other declarations or base-subtype use. Duplicate
clauses, mappings on Boolean or Character descendants, codes outside signed 32 bits,
null choices, mappings whose literal list is unknown in a generic contract,
and mappings following subtype declarations against an earlier
private view are diagnosed. General Ada freezing and wider codes remain open.

Scalar expressions and the Ada scalar call ABI carry positions. Typed stores
encode positions; typed loads decode codes, including atomic/volatile storage.
Readonly code tables belong to each emitted unit; the runtime decoder uses
binary search and raises Constraint_Error for an unknown code. Composite copies
retain stored codes. This keeps Pos/Val/Succ/Pred, comparison, array bounds,
indexing, case selection, generic operations and enum loops positional.

Imported C scalar values and results carry codes; imported Ada scalar values
and results carry positions. Writable imported scalar actuals use checked
storage and copy back only after normal return and successful decoding/range
checks. Pointer and composite imports retain the existing native-storage ABI.
Scalar stream Write/Output use stored codes; Read/Input validate codes and
subtype bounds before updating the destination. Composite streams retain the
existing representation-copy behavior rather than adding general type-aware
streaming in this step.

## Size and storage

Static tagged Size padding is supported by the
[Phase 6f contract](tagged-discriminants.md), including parent-prefix preservation.
The untagged representation rules below still apply to their original subset.

Size clauses accept positive byte multiples. Discrete scalar storage uses 8,
16, 32 or 64 bits (enumerations stop at 32); floating widths must still match
digits, and fixed/access storage remains 64 bits. A requested width must hold
the static subtype range, including the corresponding codes for a mapped enum
subtype. Null and dynamic scalar ranges cannot change storage.

AIDA also permits `subtype Byte is Long_Integer range -128 .. 127;` followed
by `for Byte'Size use 8;`. **This syntax is an AIDA extension:** Ada 2005
RM 13.3(48) permits specification on first subtypes. It was already accepted
for confirming sizes; the extension now changes named-subtype storage too.
Size clauses for untagged records follow the record layout freezing rules
below. Other size-changing clauses retain the adjacency requirement. A
subtype-specific Size preserves parent and sibling storage.

Storage width and arithmetic width are separate. Byte above occupies one byte,
but its expressions, arithmetic checks and scalar Ada ABI retain Long_Integer's
64-bit range. An Integer subtype stored in eight bytes still uses Integer's
32-bit arithmetic. Loads extend and stores narrow at memory boundaries;
parameter slots, captures, imported writable actuals and atomic storage use the
same conversion. Enum expressions continue to carry positions. Component
layout uses subtype storage, so arrays of Byte have byte strides and records
align subsequent fields using the selected component sizes.

For static, complete, untagged noncontrolled composites, Size can add trailing
storage or remove previously inherited trailing storage. It cannot shrink below
the existing component layout or choose a size not divisible by alignment.
Size clauses preserve field, variant and array-element offsets. Record component
clauses below can choose field offsets; bit packing and dynamic layouts remain
separate work. Changing
sizes of limited, task/protected or controlled composites is diagnosed.
Confirming clauses retain the existing behavior.

Composite allocation uses the selected full storage size, including globals,
heap objects, managed large arrays, temporaries and result buffers. Copies move
the component layout rather than reading a larger subtype's trailing storage
from a smaller object. Differently sized static composite formals get temporary
storage, with constraint checks and normal-return-only copy-back. This is the
implementation's copy choice for these ordinary composites. Conversions and
inherited result profiles/buffers retain the selected storage. Static array
padding belongs to its exact shape; new bounds do not inherit an obsolete total
size. Slice views exclude the parent's trailing storage and use component
alignment, since their first component may start at an interior offset.

Scalar streams use the attribute prefix's storage format, then check the actual
destination subtype before copy-back. Record streams retain native storage;
array streams retain contiguous component data, omitting trailing array storage.
Static array Input allocates the full subtype storage before reading that data.
This remains representation-oriented streaming, not general Ada stream support.

## Record components

Ada 2005 RM [13.5.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-5-1.html)
and [13.5.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-5-2.html)
are the baseline. Complete, visible, untagged record first subtypes can specify
some, all or none of their components, including discriminants:

```ada
type Pair is record
    A, B : Integer;
end record;
for Pair use record
    B at 0 range 0 .. 31;
    A at 4 range 0 .. 31;
end record;
```

Position and bit bounds must be static integers. Position and first bit must
be nonnegative; last bit may be first bit minus one for empty storage. Offsets,
extents and final alignment padding are checked for overflow. A first bit that
is a byte multiple is normalized into the byte position. Naturally aligned
integer and
enumeration components can use smaller 8/16/32/64-bit storage when supported by
the existing scalar load/store model and all subtype values fit (enumerations
stop at 32 bits). Their arithmetic and scalar call ABI retain the original type.
Ordinary slots for other components require at least their existing storage
size and natural alignment. A larger ordinary slot reserves trailing bytes
without changing the value's load/store format.

Repeated/unknown component names, duplicate clauses, insufficient storage,
alignment violations and overlapping components that can coexist produce
diagnostics. Distinct alternatives of the same variant part may overlay;
nested alternatives inherit their enclosing exclusions. Common components and
a nested variant's parent components must remain distinct. Omitted components
are placed after all specified slots, in declaration order. Derivation and
subtype aliases inherit settled layouts, including narrowed scalar fields.
A confirming clause on a derived type preserves inherited positions.

Component placement is settled before Size and Alignment within the same
representation group immediately after the type declaration. Elsewhere,
items are processed in source order. Size must cover the final layout and
respect its alignment. The obsolescent `at mod N;` form supports 1/2/4/8
alignment without weakening component alignment.

`Position`, `First_Bit` and `Last_Bit` accept selected record components without
arguments and evaluate their enclosing object once. They report normalized
byte position and the first and last bits relative to that byte. Without a
component clause, the existing component layout is reported. A scalar
component's Size reports its reserved slot. Array component Size continues to
report the actual per-object payload extent. Storage-place queries for
separately allocated dependent array payloads are explicitly diagnosed.

### Packed and unaligned discrete components

Adjacent clauses on untagged records can place nonaliased, nonshared integer,
modular and enumeration fields with static bounds in slots of 1 to 64 bits.
This includes discriminants, fields spanning several bytes, and byte-aligned
fields at offsets that do not satisfy the scalar's ordinary alignment. The
slot must hold every subtype value, using enumeration representation codes
when present. Signed fields use two's complement. Bit zero is the low bit of
the first byte, with significance increasing through successive bytes.

Coexisting components may share a byte when their bit intervals are disjoint.
Writes preserve bits outside the slot, and variant exclusions still allow the
same bits to be reused by different alternatives. Components sharing a byte
require synchronization for concurrent updates to the same object. The packed
or unaligned component has byte alignment and reports the slot width for
`Size`. Its enclosing record still honours the alignment of ordinary
components and any explicit record alignment.

Scalar reads and writes, renamings, generic `in out` views and scalar call
copy-back access the slot through byte operations. Streams keep the normal
component format and use these operations when reading or writing the object.
Address, Access and Unchecked_Access on these fields are diagnosed. Aliased/shared fields,
nonstatic scalar bounds and packed or unaligned nondiscrete fields remain
outside this subset.

### Discriminant-dependent arrays

A record clause changes direct dependent arrays from object-owned descriptors
to inline slots. Their maximum extent is conservatively bounded from static
component bounds and the discriminants' declared scalar ranges. Each specified
slot must cover that maximum; omitted dependent components receive such a slot
automatically. For example, a discriminant subtype `Natural range 0 .. 6` allows
`String (1 .. N)` in a six-byte slot. Bounds with no static maximum, including
nonstatic captured bounds, remain diagnosed. One- and multidimensional arrays
use the same rule.

The record has fixed size and contains its payload directly. Component array
views still derive bounds and strides from the enclosing object's current
discriminants. Construction checks the actual extent against its slot before
writing. Default/aggregate initialization, mutable variant assignment, copies,
function results, array elements and allocations therefore retain independent
values without descriptor ownership or a new runtime allocator. Controlled
components elsewhere in the record keep their existing lifetime registrations.

### Changed layouts on untagged derived records

A record clause can move inherited fields and change their storage widths
within the same component-clause rules. When a clause changes placement,
omitted fields follow the specified slots in declaration order. A confirming
clause retains omitted fields at their inherited positions.

Value conversions between related types copy fields into the target layout,
even when the two records occupy the same number of bytes. Conversion reads
the active variant from the source discriminant and checks target discriminant
constraints on the converted value. Inherited defaults and aggregate boxes read
discriminants from the actual layout and storage width. Inherited operations
receive storage in the parent layout when necessary. Writable actuals are evaluated once, and
copy-back converts to the actual layout after normal return. Inherited
results, including custom stream Input results, are converted to the caller's
layout. An inherited custom Read uses the same writable argument adaptation.

This subset covers nonlimited, unmanaged, fixed-layout records, including
ordinary discriminants and variants. Aliased components, shared components,
private or tagged subcomponents, and dependent component bounds retain
diagnostics. Inherited anonymous access profiles also retain a diagnostic
because a pointer would require a shared view across layouts. AIDA also permits
repacking derived types whose parents have user-defined primitives, as an
extension to Ada 2005 RM 13.1(10).
[Ada 2005 RM §13.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-1.html)

### Record layout freezing

A record placement, Size or Alignment clause can follow intervening
declarations in the same declarative part, including in the private part of
the package specification that declares the record. Subtype marks in plain
subtype, access type and callable profile declarations leave the layout open.
A fresh record subtype can have its own Size even when its base type is frozen.

A clause must precede layout use. The front end tracks object creation,
allocators, component selection, layout attributes, resolved calls and generic
type actuals. It also settles preceding layouts at body boundaries and settles
package layouts after the private part. Freezing propagates through record
components, array elements and ancestors. Because AIDA computes dependent
layouts eagerly, a record used as a field, array element or parent is treated
as frozen when the containing or derived type is declared. A subtype-specific
Size also settles its base layout.

This is a record layout subset of Ada freezing. Deferred defaults can still
freeze conservatively. General callable, tagged and generic freezing rules and
nonrecord representation items retain separate follow-up work.
[Ada 2005 RM §13.14](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-14.html)

### Remaining record restrictions

Tagged/task/protected placement, declaration-sized runtime layouts, nondefault
bit order and Pack remain open. Dependent arrays of controlled/task/protected
elements retain their existing restriction. The Phase 6f tagged Size subset is
linked above.

## Evidence

`alignmentclauses` checks attributes, inheritance, nested field padding, array
strides, empty composites, heap objects, captured locals, function-prefix side
effects and concurrent tasks. A C helper checks actual address divisibility.
The test runs in whole and separate compilation and checks deterministic IR.
`alignmenterrors` pins legality and subset-limit diagnostics. The existing Size,
representation rejection and ACATS suites remain regression coverage.

ACATS CD30002 is the relevant alignment case. Its Impdef/Storage_Elements support,
object clauses and multiple library units require further compiler/harness work;
see `tests/acats/README.md`. It is not counted as passing coverage.

`enumrepresentation` checks signed/unsigned storage widths and endpoints through
a C memory observer, globals, heap objects, variants, inherited/replaced mappings,
character literals, generic calls, captured parameters, positional attributes,
loops/indexing and imported scalar values/results/copy-back. `enumstreams`
checks malformed codes and subtype failures without changing the target;
`enumtasking` exercises atomic mappings and calls from four tasks. These run in
whole/separate modes with deterministic IR. `enumrepresentationerrors` pins
legality and subset-limit diagnostics. ASan/UBSan pass for the runtime test.
`runtime.enumeration` tests binary-search
boundaries, gaps, invalid positions, character case and isolated per-thread errors.

Unchanged ACATS CD3014A and CD3014D pass for ordering, array indexing and
generic instantiation, including mappings inside a generic package. These tests
check positional language behavior; the local C observer separately verifies
that objects contain the requested codes. Broader representation ACATS cases
requiring Unchecked_Conversion or general freezing remain unselected.

`sizestorage` checks physical widths, narrow subtype field offsets, padded
records/arrays, multidimensional and nested strides, global/heap/large local
allocation, inherited profiles/results, conversions, slice views, arithmetic
width, canaries around copied actuals and imported scalar/composite copy-back.
`sizestreams` checks prefix formats and failed reads; `sizetasking` exercises
narrow/wide atomic scalars from four tasks. All run in whole/separate modes with
deterministic IR. `sizestorageerrors` and the updated `sizelayouterrors` pin the
remaining restrictions. `semantics.type_storage` tests the layout/width contracts.
ASan/UBSan pass for that unit test and the storage program with an instrumented
runtime and C observer (QBE-generated accesses remain uninstrumented).

ACATS CD2A22A and its official unmodified Length_Check helper are vendored as an
explicit compile XFAIL for unsupported Pack/bit storage. This is a tracked gap,
not an upstream pass; the normal ACATS suite and its strict XFAIL grading remain
required. Other reviewed Size cases require packing, unchecked conversion,
implementation substitutions or facilities outside this step.

`recordrepresentation` checks actual component addresses and C-observed bytes,
partial/reordered/normalized clauses, narrowed scalar fields and calls, Size
before/after placement, globals, captures, derived layouts, generic swapping,
heap objects and four concurrent tasks. `recorddependent` covers inline one-
and two-dimensional arrays, mutable discriminants, nested variant storage,
copy independence, derived types, arrays of records, results and deallocation.
`recordcontrolled` checks overlaid controlled components, assignment failure,
recovery and cleanup. All run whole/separate compilation with deterministic IR.
`recordrepresentationerrors` pins legality and subset limits; the storage unit
test checks explicit extents and nested variant exclusivity.

The full suite passes. The focused frontend/unit/ACATS checks pass with UBSan;
the three execution regressions pass whole/separate with an ASan/UBSan runtime
and C observer. The four-task placement test also passes whole/separate with
a ThreadSanitizer runtime. QBE-generated memory accesses remain uninstrumented.
Unchanged ACATS CD4051A and CD1C03H pass placement and inherited storage-place
requirements. Unchanged CD4031A passes packed variant overlap and inherited
storage-place checks. `recordbits` covers exact physical bytes, cross-byte
signed and unsigned access, mapped enumeration values, imported copy-back,
renaming, generic views, custom streams and four tasks. It runs whole/separate
with deterministic IR. The rejection test covers overlapping bit intervals,
invalid widths, dynamic bounds, aliased/shared fields and address escapes.
Focused compiler/C-runtime ASan/UBSan and C-runtime ThreadSanitizer checks
pass. These runs instrument the compiler and C runtime as applicable, while
the generated QBE accesses execute as native assembly. Upstream provenance is
in `tests/acats/README.md`.

Unchanged CD4051B and CD1C04E pass reordered and replaced inherited placements.
`recordrepack` covers equal-size swaps, partial clauses, sibling conversions,
multi-level inherited results, inherited `in`, `in out` and `out` operations,
packed variant transitions, inherited defaults and boxed components, constrained
and exceptional calls, and inherited stream operations. It runs whole/separate and deterministically, with four
tasks executing the call and conversion checks.

Unchanged CD1009N passes placement in visible and private package parts.
`recordfreezing` covers nonadjacent placement, Size and Alignment, subtype
views, function results and allocations in whole/separate compilation and
deterministic IR, with four concurrent callers. `recordfreezingerrors` pins
the freeze checks for earlier layout uses and intervening bodies.
