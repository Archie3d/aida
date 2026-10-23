# Runtime array components and allocation

Phase 6b extends the shared [array descriptor contract](array-descriptors.md).
The language baseline is Ada 2005 RM
[3.6](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-6.html),
[3.8](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-8.html),
[4.8](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-8.html)
and [13.13.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-13-2.html).

## Captured constraints and contiguous layouts

Named runtime-constrained array subtypes and anonymous array constraints can
serve as record and array components and access designated subtypes. They must
be definite: an unconstrained array component without bounds is still illegal.
Constraints are elaborated with their type declaration, independently for each
component in a grouped declaration. Scalar and array constraint plans preserve
source order. Defaults run for each object, without reevaluating its captured
component constraints. Bounds depending on discriminants remain Phase 6c work.

Arrays retain contiguous component storage. Their byte strides now include
runtime component sizes, including arrays of arrays and arrays of records.
An untagged runtime-sized record captures its padded size and every component
offset in a declaration-owned layout table. Variant alternatives share their
parent cursor, with enough storage for the largest alternative. Alignment and
addition are checked before offsets or sizes are stored. Length/product,
layout-addition and allocation capacity failures raise Storage_Error.

Layout tables live in the owning activation frame or exported library data.
Anonymous constraint names include source filename, location and semantic scope,
so spec/body declarations at identical positions cannot collide. Recursion and
separate tasks retain independent local captures; clients in other units use
the library table. Objects, aggregates, results and captured variables use
managed storage where their size is dynamic. Selection, equality, copying,
defaults and controlled walks use the same size/offset helpers. Existing
master, temporary-arena and active-element finalization rules still apply.

## Allocators and access values

Array allocators support runtime named constraints, explicit per-evaluation
bounds, inferred initializer bounds, null arrays and multiple dimensions.
Explicit bound expressions and initializer function calls run once per
allocator evaluation. Qualified aggregate bounds are prepared once before
component initialization. A constrained designated subtype checks shape and
slides initialization to its bounds; otherwise the created array retains the
allocator's bounds. Existing scalar and record allocator checks remain.

Access values remain one pointer. Each allocated array saves a complete array
view in its existing collection allocation record's arena. Dereferencing an
access-to-unconstrained-array value retrieves that saved view, rather than
consulting a later allocator's bounds. Constrained designated subtypes can
use their captured shape directly. Assignment and null comparison retain the
pointer ABI, including derived access types and generic deallocation.

Construction, descriptor metadata and controlled parts share the allocation's
lifetime. Unchecked_Deallocation and collection finalization reclaim all of
them. Inferred limited array results construct under a pending collection owner;
failed construction aborts that allocation. Partial initialization finalizes
only active components. Metadata lookup uses the existing synchronized
collection index; object publication and access still require Ada program
synchronization. No new mutable C global is added.

## Size and streams

Subtype and object Size use captured runtime sizes, with checked multiplication
by eight and contextual integer narrowing. Object prefixes are evaluated once.
Static Size clauses still require a complete static layout.

Array Read/Write transfer contiguous payload without bounds. Array Output/Input
transfer two signed 64-bit bounds per dimension before the payload, including
constrained arrays. Input returns the type's first subtype; a constrained
subtype prefix does not constrain an otherwise unconstrained result. Index-base,
subtype, shape and storage checks occur before allocating the input buffer.
Record streams retain the existing raw representation format, now using the
runtime record size. General component-wise, controlled, tagged and portable
streaming remains Phase 7 work.

This corrects the former omission of bounds for constrained array Output/Input;
regenerate streams written with that old format. The existing unconstrained
rank-one bound format and internal array call/result descriptor ABI are unchanged.

## Limits and validation

Runtime-sized tagged records and layouts requiring a private component's full
view after the enclosing type declaration remain explicitly diagnosed. Their
layout/tag completion work belongs to Phase 6f. Discriminant-dependent bounds,
controlled variants, general limited records, custom storage pools and foreign
unconstrained-array descriptors remain separately tracked. Fully defined private
completions, ordinary nested records and supported controlled/limited component
lifetimes work through the existing mechanisms.

`runtimecomponents`, `runtimeallocators`, `runtimecomponentstreams`,
`runtimecomponentcleanup`, `runtimecomponenttasking` and `runtimecomponentchecks`
cover grouped/interleaved captures, recursive frames, private completions,
library/generic layouts, dynamic protected private fields, aligned offsets,
variants, nested arrays/records, sliding,
nulls, all-dimensional allocation, stream interoperability, limited results,
partial failure and four-task allocation followed by cross-task deallocation.
They run whole/separate and deterministic-IR checks. Rejection tests retain
indefinite-component, static-representation, tagged and late-completion limits.
`runtime.dynamic_layout` checks native alignment/overflow and descriptor ownership.
Unmodified ACATS C48005B checks one- and two-dimensional allocator bounds.
ASan/UBSan and ThreadSanitizer instrument the native helper and the C runtime
used by the Ada mode tests; frontend UBSan checks supplement them. Generated
QBE accesses remain uninstrumented, so these are not complete Ada race checks.
