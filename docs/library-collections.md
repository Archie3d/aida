# Unbounded strings and basic vectors

Phase 7's first library increment is implemented in Ada using controlled
objects and the existing shared access-type collections. The target contracts
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

## Ada.Containers and Ada.Containers.Vectors

The root defines `Hash_Type` with modulus 2**32 and `Count_Type` ranging from 0
to Integer'Last. The generic accepts a signed integer index subtype, a definite
nonlimited element type and an equality actual. A vector has a tagged private
view with controlled ownership. `Extended_Index` and `No_Index` follow the
index subtype; the first index is its lower bound, including negative bounds.
As required by A.18.2, the actual's first bound must permit a predecessor in
its base type. The normal generic legality checks reject indefinite and limited
elements. Private implementation components remain inaccessible to clients.

Supported operations are equality, both `To_Vector` constructors,
`Capacity`/`Reserve_Capacity`, `Length`/`Set_Length`, `Is_Empty`/`Clear`, indexed
`Element`/`Replace_Element`, first/last indices and elements, element insertion
and prepending with counts, append of elements or vectors, indexed and first/
last deletion, swap, reverse and move. The specification is the exact API list.
Missing operations are diagnosed through ordinary name/profile resolution.
Cursor APIs, callback traversal/update, searching, sorting, vector insertion/
prepending, concatenation, indefinite vectors and other families belong to the
next Phase 7 container item. Without cursor/callback APIs there are no live
cursor or callback tampering intervals. This increment does not implement the
full container tampering model or streaming.

The vector owns a geometrically grown table of pointers to separately allocated
live elements. Reserving capacity neither initializes spare elements nor copies
existing elements; deleting and clearing finalize only removed elements.
Copying deep-copies live elements and uses the supplied equality operation.
Move transfers ownership and empties the source; self-move and self-assignment
preserve the vector. Self-append takes an owned snapshot. Indexed operations
validate bounds before touching elements; growth beyond the index range raises
Constraint_Error. Capacities also have to fit that range. Allocations retain
the existing runtime's Storage_Error behavior.

Insertion builds pending elements before changing the target. Failed element
initialization/adjustment unwinds partial construction. Vector Adjust similarly
detaches the borrowed table and releases a failed partial copy. Clear detaches
each element before calling its finalizer, completes all remaining deallocations
on failure, then raises Program_Error. Vector finalization frees the table even
when an element finalizer fails. Existing controlled assignment rules still
apply to replacement and assignment failures; strong rollback is not promised
for a user element's failed Adjust/Finalize.

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
