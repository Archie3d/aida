# Runtime discriminants and per-object components

The supported untagged record subset follows Ada 2005 RM
[3.7.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-7-1.html),
[3.8](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-8.html)
and 5.2. It extends the [default-discriminant contract](default-discriminants.md)
and reuses the [array descriptors](array-descriptors.md), owned-result ABI,
collection ownership and finalization chains.

## Constraint evaluation

Discriminant constraints accept runtime discrete expressions, positional,
named, mixed and grouped associations. Every discriminant must receive one
compatible value; positional associations precede named ones. Grouped
expressions are evaluated once per associated discriminant.

Named subtype, record-component and array-component constraints capture their
values when their declarations elaborate. Grouped object declarations capture
anonymous constraints independently immediately before each object's
initialization. Allocator constraints evaluate for each allocator evaluation.
Each value is checked against its discriminant subtype, including captured
runtime scalar bounds, at constraint elaboration. Aliases and derived types
preserve the original capture; library captures are exported for separate
compilation. No constraint is silently reevaluated from a later variable value.

A component constraint can instead depend on a containing record's
discriminants. Such discriminant names must appear alone; a scalar component
cannot be constrained by a discriminant (RM 3.8). For example:

```ada
type Buffer (Length : Natural := 2) is record
    Text : String (1 .. Length) := (others => 'x');
    Count : Natural := Length;
end record;
```

The discriminants are initialized before dependent constraints and defaults.
One- and multidimensional array bounds and nested record discriminant
constraints are supported. Non-dependent expressions in a per-object
constraint are captured at type elaboration, independently for each grouped
component; only the discriminant expressions are evaluated for each object.
Index-base and non-null index-subtype checks precede checked extent/stride and
allocation arithmetic. Null components retain their bounds and allocate no
payload elements. Size-capacity failures raise Storage_Error.

## Object representation and ownership

An untagged record with a direct discriminant-dependent array component has a
stable wrapper. Two leading pointer-sized words hold its component-storage
arena and, when needed, its declaration-owned layout table. Each dependent
array field stores the existing array view: data pointer and four signed
64-bit values per dimension (first, last, length, byte stride). Payload is
allocated through the existing checked array arena helpers. Records can mix
these views with ordinary declaration-sized components; views in different
objects retain independent bounds and data.

Default and aggregate construction initialize the discriminants before
allocating the applicable components. Mutable unconstrained assignment can
change lengths and active variants. Assignment checks target discriminants
before changing storage, then uses the existing adjusted snapshot/copy path
to protect aliases. Dependent payloads are copied rather than shared. Callback
arguments, array elements, nested components, allocators and owned function
results use the same representation.

Internal storage finalizers register on existing local, temporary, collection
or library chains. They do not invoke user Initialize/Adjust/Finalize for the
plain record. Each arena is active before its first component allocation;
failed defaults and copies release completed allocations. Common controlled
components retain their existing registrations and user callbacks. Collection
allocation/deallocation can cross tasks under the existing synchronized
ownership registry; object accesses still need ordinary Ada synchronization.
No new mutable C global or QBE modification is required.

`Size` describes the wrapper representation, including descriptors and padding;
it does not count separately allocated payload. Array component Size describes
its payload as before. Existing raw record/array streaming is explicitly
rejected for composites containing these managed records, to avoid serializing
pointers. Streaming a component through its ordinary array type remains
available; general component-wise record streams remain Phase 7 work.

## Remaining limits and evidence

Dependent arrays with controlled, task or protected elements are diagnosed.
Controlled components in variant records remain the next Phase 6c item.
Tagged composites containing managed dependent records and general limited
record definitions retain their Phase 6f restrictions. Discriminant constraints
on access subtypes remain unsupported; access types designating a constrained
record subtype and record allocator constraints are supported. Record
representation clauses remain the later Phase 6c item.

`runtimediscriminants`, `discriminantcomponents`, `discriminantcleanup` and
`discriminanttasking` cover evaluation timing, grouped/named constraints,
runtime scalar discriminant subtypes, alias/derived captures, per-object
multidimensional and nested-record bounds, nulls, defaults, copies, callbacks,
results, protected calls, partial failure, overflow and cross-task deallocation.
They run whole/separate compilation and deterministic-IR checks, including
exported library captures and four concurrent tasks. `discriminantcomponenterrors`
pins legality and unsupported-feature diagnostics; `semantics.type_storage`
checks wrapper layout and resource/layout classification.

Unchanged ACATS C37211A checks runtime discriminant constraint/default range
failures in subtypes, components, objects and allocators. C37108B checks failed
per-object array and record constraints, including private and limited-private
views. These are selected passes, not a claim of complete conformance.

The frontend unit, rejection and determinism checks also run under UBSan.
Cleanup/default-failure and overflow scenarios pass with an ASan/UBSan C
runtime; the four-task shared-collection/protected-call scenario passes with
ThreadSanitizer in whole and separate modes. QBE-generated memory accesses
remain uninstrumented.
