# System.Storage_Pools (Ada 2005)

The source package implements the [13.11 profile](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-11.html):
Root_Storage_Pool is abstract, limited controlled and preelaborably initialized;
Allocate, Deallocate and Storage_Size are abstract dispatching primitives.
Concrete descendants provide ordinary Ada overrides, including library-level
operations and nested operations with captured lexical environments. The compiler
validates the predefined root and primitive profiles before enabling the ABI.

## Association and queries

A Storage_Pool clause on a nonderived named access-to-object first subtype
captures a variable pool view once at clause elaboration. Constants, subprogram
access types, subtypes, derived targets, duplicate clauses, late clauses and
conflicting Storage_Size specifications produce diagnostics. Pool names can be
class-wide dereferences, and another type can share T'Storage_Pool. Changing the
access value used in a pool name later does not change the captured pool.

Derived access types and subtypes use their ultimate ancestor's pool and
collection. Access derivation itself does not freeze the parent; allocators and
uses do freeze the ancestry, preventing a pool change after allocation.
T'Storage_Pool is a variable Root_Storage_Pool'Class view. T'Storage_Size calls
the pool's dispatching Storage_Size function each time. These attributes require
an access-to-object subtype prefix. Their predefined package dependency is
loaded for queries even without an explicit with clause.

A custom collection is registered at the association clause, after evaluating
the pool name. Thus its cleanup precedes pool object finalization even if the
access type was declared before the pool object. Pool objects and their captured
environments must remain alive throughout use of their associated collections.
No pointer or pool name extends an object's accessibility or lifetime.

## Allocation, cleanup and concurrency

Each allocator calls Allocate with the actual contiguous storage extent and
designated alignment. Scalars use their selected storage representation; arrays
use their evaluated shape, including zero-length and packed allocations;
specific tagged objects use their native extent. Nonlimited class-wide
allocators with a concrete initializer use its tag's size and alignment. Current
compiler-supported alignment requests are 0, 1, 2, 4 and 8 storage elements; zero
denotes no alignment promise. Pool-returned storage is cleared before Ada
initialization, matching the existing default allocator behavior.

Allocation records retain the pool, byte extent and alignment through collection
membership, array bounds, task activation and controlled-part registration.
Unchecked_Deallocation detaches the record, waits for allocated tasks and
finalizes components, then calls the original pool's Deallocate with the saved
extent/alignment. Remaining objects follow the same path at collection cleanup.
Pool storage is never passed to free by the collection runtime. Initialization
failure releases initialized parts and returns the block through the pool.
An exception from Allocate propagates unchanged; a pool that throws after
acquiring storage must clean up its own unsuccessful acquisition.

Deallocate executes with a clear pending exception status, including when an
initializer has already failed. The complete original occurrence, message,
origin and trace are preserved across that cleanup call. A normal explicit
Deallocate exception propagates; during collection finalization, existing
finalizer failure rules apply. A null or improperly aligned successful Allocate
result is diagnosed at run time with Program_Error; a nonnull misaligned block
is returned through Deallocate without initializing it. This is a check on a
subset of erroneous pool behavior, not proof of storage validity or extent.

Shared-storage locks protect allocation indexes and collection membership.
Allocate, Deallocate, Storage_Size and object finalizers run without that lock;
callbacks can reenter allocation/deallocation. Pools are not automatically
serialized: applications must synchronize mutable pool state and publication,
object access and deallocation. Workers execute callbacks in their own bound
task context. A collection belongs to the master of its access type, preserving
the existing task/master and cross-task deallocation rules.

## Default policy and remaining limits

Unspecified pools retain native calloc/free allocation and the existing
per-access-type collections. Queries expose a shared implementation default pool
object; associating another access type with that object uses the same native
allocator, while each unrelated access type retains its own collection lifetime.
The default Storage_Size query reports Storage_Count'Last as an implementation
capacity indication, not pre-reserved memory. Static zero Storage_Size retains
its allocator prohibition, including derived types; its query returns zero.
Positive Storage_Size clauses remain explicitly unsupported.

Custom pools currently reject coextension allocators, discriminant-dependent
owned component storage, dynamically tagged initializer values, and limited
array results whose size is unknown before construction. Known-size limited
construction, controlled objects, task objects, runtime-bounded arrays and
concrete tagged/class-wide initialization use the pool. The rejected arena-result
paths continue to work with default collections; they never silently bypass a
specified pool. Internal registration records, array descriptors, controlled
component metadata, task records and temporary arenas use implementation storage
outside user pools. Their lifetime/cleanup remains with the owning allocation.

[Max_Size_In_Storage_Elements](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-11-1.html)
retains the implementation from Storage_IO: native extent for complete definite
subtypes, including captured runtime layouts; unconstrained/class-wide and
dependent-storage prefixes remain diagnosed. These limits are explicit
implementation gaps; unrestricted storage-management conformance is not claimed.

## Evidence

Release validation passes all 1,834 tests. All 25 focused ASan/UBSan checks pass,
including allocation regressions and malformed-input recovery; both selected
ThreadSanitizer runtime checks pass.

`storagepools` covers captured/shared/default/derived pools, library package
elaboration, private callback environments, scalar/array/class-wide requests,
zero alignment and empty arrays, zero Storage_Size, controlled initialization
failure and scope cleanup, task completion before deallocation, callback
exceptions and four workers. Whole/separate compilation and deterministic IR are
checked. Legality and malformed-root controls require normal diagnostics.

`runtime.storage_pools` runs four native workers with independent task contexts
against one shared pool. It checks exact size/alignment pairs, reentrant
callbacks, cleared storage, refused/null/misaligned allocation, callback failure,
pending occurrence preservation and collection cleanup. ThreadSanitizer also
covers this test and the existing shared-collection runtime control.

Unchanged ACATS C3A0015 passes its complete REPORT scenario for parent/derived
pool identity, generic allocators, conversions and deallocation using default
and user-defined pools. Original CDB0A01 and FDB0A00 are reviewed and pinned but
still compile-rejected at its positive Storage_Size clause; that observation is
not a conformance pass. The [manifest](../tests/acats/storage_pools/manifest.json)
records original paths, SHA-256/Git-blob hashes and Ada 2005 eligibility at ACATS
revision `2785c30c1f8577d97ac7b3043882f6818781e9c5`. CDB4001's Ada 2012 subpools
are outside this Ada 2005 item.

All 145 original backfill outcomes remain unchanged. Two ungraded B cases,
B34001B/B34002B, now reject invalid Storage_Size prefixes with the implemented
attribute's legality diagnostic instead of an unsupported-attribute diagnostic.
