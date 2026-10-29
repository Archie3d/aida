# Unbounded strings and Ada 2005 containers

Phase 7's string and container increments are implemented in Ada using
controlled objects and the existing shared access-type collections. The target contracts
are Ada 2005 [A.4.5](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-A-4-5.html)
and [A.18.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-A-18-2.html).
This is a documented subset, not complete predefined-library conformance.

## Ada.Strings.Unbounded

`Unbounded_String` has a private, untagged view, defaulting to the empty string.
A hidden controlled component owns its allocated string. Adjust copies the
storage, first detaching the shallow pointer so an allocation failure cannot
release the source. Finalize frees storage. No reference counts or new C globals
are introduced. Separate objects may be copied, mutated and finalized by
separate tasks using the shared library collection; callers must synchronize
concurrent access to the same object.

The package supplies conversions (including the length constructor),
`Set_Unbounded_String`, `String_Access`/`Free`, append/concatenation, element and
slice operations, comparisons, search/count/token operations, mapping-value
translation, replacement/insertion/overwrite/deletion, trimming, head/tail and
repetition. `To_String` normalizes its lower bound to 1; `Slice` preserves Low.
Public string indices and lengths use Positive/Natural. Bad element/slice
indices raise `Ada.Strings.Index_Error`; fixed-string helpers supply their
existing pattern, index and translation checks. Mutations construct replacement
values before assignment; checks and construction failures preserve the source.
The length constructor fills its unspecified characters with NUL.

Storage is proportional to the string length. Append allocates a new value;
there is no amortized append-capacity guarantee. Length is limited by Natural,
addressable storage and the existing signed-32-bit string-result ABI. Mapping
function overloads remain with the later Phase 7 mapping-callback item;
wide strings and categorization enforcement retain their separate owners.

## Ada.Containers families

The root defines `Hash_Type` with modulus 2**32 and `Count_Type` ranging from 0
to Integer'Last. The Ada 2005 APIs are provided by all twelve families:

| Definite elements | Indefinite elements/keys |
| --- | --- |
| Vectors | Indefinite_Vectors |
| Doubly_Linked_Lists | Indefinite_Doubly_Linked_Lists |
| Hashed_Maps | Indefinite_Hashed_Maps |
| Ordered_Maps | Indefinite_Ordered_Maps |
| Hashed_Sets | Indefinite_Hashed_Sets |
| Ordered_Sets | Indefinite_Ordered_Sets |

All names above are children of `Ada.Containers`. Their specifications are the
exact API inventory. They include cursors, query/update/iteration callbacks,
copying, ownership transfer, sequence insertion/deletion/search, list splicing,
map lookup and replacement, set algebra and the nested `Generic_Keys` views.
Vectors and lists supply `Generic_Sorting`; both generic array sorting
procedures are also available. Ada 2012 additions, including bounded containers,
holders, generalized iterators, reference types, `Assign`/`Copy`, renamed vector
operations and `Generic_Sort`, are outside this Ada 2005 target.

### Ownership and element constraints

A hidden controlled store owns nodes, values and container metadata. A store's
Adjust detaches its borrowed source before constructing an independent copy;
its Finalize destroys all live nodes and releases metadata. Clear detaches each
node before invoking user finalization, completes the other deallocations if a
finalizer fails, then raises Program_Error. Deallocation of a node similarly
finishes both its key and element before propagating finalization failure.
Insertion constructs an owned pending sequence before changing the target, so
allocation/adjustment failures release partial construction. Replacement installs
its new owned value before releasing the old one. Existing controlled assignment
rules still apply to failed user Adjust/Finalize; rollback of assignment is not
promised.

The shared internal `Ada.Containers.Storage` generic implements these ownership
and cursor mechanisms once. The public families supply their storage policy:
definite elements/keys live in record components, preserving unconstrained
variables when a definite type has defaulted discriminants; indefinite values
are allocated directly, preserving their individual bounds, discriminants and
tags. Thus a definite Update_Element callback can legally change mutable
discriminants, while indefinite array callbacks may receive constrained objects.
Replacing an indefinite value can change its bounds. The internal helper is an
implementation unit, not an additional language-defined container interface.

Generic legality rejects indefinite actuals for definite families and rejects
limited elements/keys. Supported actuals retain the compiler's existing type,
representation, accessibility and generic-profile restrictions. Comparisons,
equivalence, hashing, ordering and key projections use the supplied generic
actuals; callers must satisfy their language-defined consistency contracts.

### Organization and cursors

Vectors use geometrically grown index tables for random access. Spare capacity
contains no live elements. Lists use doubly linked nodes; hashed maps/sets use
bucket chains at load factors no greater than one; ordered maps/sets use AVL
search trees with parent and linked iteration pointers. Rotations and deletion
transplant nodes without copying their keys or values. Ordered lookup and bound
queries traverse the tree. Hash iteration is deterministic insertion order;
replacing a hashed-set element retains its iteration position while rehashing
its node. Requested capacities are rounded up as necessary; capacity shrinking
is not promised.

A cursor identifies its owning state and node. Foreign-container arguments raise
Program_Error, and required No_Element arguments raise Constraint_Error. Valid
list/map/set cursors retain node identity across operations that preserve them,
including list sorting and splicing. Operations that invalidate cursors or make
vector cursors ambiguous retain the Ada 2005 rules; callers should reacquire
such cursors. There is no lifetime extension or stale-pointer validation for
invalid cursors: using a deleted/finalized cursor has the RM's erroneous or
unspecified behavior. Container copy creates independent cursor identities.
Cursor input arguments are snapshotted before writing aliased output arguments
or invoking user callbacks.

Vector indices start at the actual index subtype's lower bound, including
negative bounds. `Extended_Index`/`No_Index` follow A.18.2; the actual first bound
must have a predecessor in its base type. Growth beyond the index range raises
Constraint_Error. Empty vector elements created by Insert_Space or indefinite
length construction have no readable value: operations that read them raise
Constraint_Error, with mutating sort/merge/swap checks preceding changes. Replace
can initialize these slots. Definite length construction/default insertion may
initialize elements to their default values.

### Callbacks, sorting and tasks

Iteration prohibits tampering with cursors; Query_Element and Update_Element
protect element access as well. Locks are per container, are nested and are
released on exceptional exits. Forbidden operations raise Program_Error, even
when the operation would otherwise leave the length unchanged. Hashed
Reserve_Capacity checks cursor tampering. Value replacement during vector/list/
map iteration remains allowed when the operation does not tamper with cursors.
Generic_Keys.Update_Element_Preserving_Key saves the projected key, invokes the
callback, and removes the element with Program_Error if its equivalence class
changes after a normal return.

Vector and array sorts use heapsort; list sort uses stable bottom-up mergesort.
List sort preserves the element associated with each cursor. Merge requires
inputs ordered by the supplied comparison and transfers nodes from Source to
Target; list cursors follow their transferred nodes. Comparator failures leave
node ownership intact, with contents potentially partially reordered.

Separate objects may be allocated, copied and finalized by different tasks via
the existing shared collections. A producer/consumer regression copies and
releases whole containers allocated by other tasks. The containers do not add
synchronization for concurrent access to the same object; callers must supply
it. No mutable C globals or new foreign ABI are introduced.

Streaming remains the later Phase 7 stream milestone. Container stream attribute
calls currently diagnose controlled streaming rather than serializing pointers.
Package categorization, Preelaborable_Initialization and wide character types
retain their separate Phase 7 owners; these packages do not claim full library
conformance or complete implementation-advice compliance.

## Validation

`unboundedstrings`, `basicvectors`, `vectorlifetime` and
`librarycollectiontasking` cover whole-program/per-unit execution and deterministic
IR. They test independent copies, self operations, growth, negative/zero-based
indices, bounded index exhaustion, controlled element counts, partial failed
copies, failed finalization, exception unwinding and four workers allocating,
copying and releasing strings and vectors through shared collections.
`librarycollectionerrors` checks generic legality and implementation privacy.
Focused compiler and C-runtime ASan/UBSan checks use the same execution paths;
the four-worker test also passes whole/per-unit with the C runtime under TSan.
QBE-generated Ada memory accesses are not sanitizer-instrumented, and host
LeakSanitizer support is unavailable.

Unchanged ACATS CXA4010, CXA4011 and CXA4031 execute successfully. CXA4032 is a
pinned compile XFAIL at the missing Maps.Constants dependency, owned by its later
Phase 7 item; its executable behavior remains untested. Mapping-function case
CXA4030 is deferred with mapping callbacks. The baseline backfill observations
remain separate from these added cases.

The complete container increment adds `containerfamilies`, `containercursors`,
`containersorting`, `containersets`, `containerlifetime`, `containertasking`,
`containergenerics`, `containerdiscriminants`, `containerstorage` and
`containererrors`. They cover all families, mutable definite versus constrained
indefinite storage, generic actual selection, array keys and class-wide values, cursor aliasing and ownership,
callback cleanup/tampering, set algebra and key changes, bounds near the integer
limit, partial failed copies, cross-task destruction, and independent AVL
ordering/height/balance/parent/size invariants across 513 insertions and a
permutation of 513 deletions. All executable scenarios run whole/per-unit and
with deterministic IR checks. Focused ASan/UBSan covers the compiler and C
runtime, and TSan covers the C runtime during the four-worker transfer test;
generated Ada accesses and host LeakSanitizer retain the limits above.

All twelve unchanged Ada 2005 Amendment ACATS CXAI001–CXAI006 and
CXAIA01–CXAIA06 are executable PASS cases, using unchanged FXAIA00 for the
indefinite families. Sources come from official modification set 3.1DD, whose
Ada 2012-only features are already disabled upstream. The newer ACATS 4.x forms
were not substituted. [Provenance and checksums](../tests/acats/README.md).
