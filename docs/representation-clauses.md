# Representation clauses

Ada 2005 RM 13.1 and [13.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-3.html)
and [13.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-4.html)
are the baseline. Phase 5 implements alignment and enumeration mapping;
size-changing subtype/composite layouts remain roadmap items. Record clauses
follow discriminated records in Phase 6c.

## Alignment subset

`for T'Alignment use N;` supports complete, visible, untagged first subtypes
without task or protected parts. N is a static integer: 0, 1, 2, 4 or 8 storage
elements (one element is eight bits). Zero removes the subtype's alignment
promise; allocation still uses the natural alignment. Positive requests cannot
weaken natural alignment. Values above eight, non-power-of-two values, duplicate
clauses and unsupported targets produce diagnostics.

Representation items must form an adjacent group after the type declaration,
before any declaration can capture its layout. This conservative restriction
stands in for general Ada freezing. Clauses on later subtype declarations or
individual objects are unsupported, as are tagged/task/protected representations.

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
