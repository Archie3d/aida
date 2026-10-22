# Array descriptors (internal ABI v2)

Array bounds are signed 64-bit values. Lengths, byte strides, offsets and byte
sizes are nonnegative signed 64-bit values, limited to `Long_Integer'Last`.
The former `Integer'Last` length limit and 32-bit multidimensional bound limit
are removed. Index subtypes still impose their own language constraints.
[RM 3.6](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-6.html)
defines dimensions; [RM 4.1.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-1-2.html)
defines slice bounds and checks.

## Shared contract

`common/Array.h` owns the compiler/runtime layout. Each dimension is four
64-bit words: `first`, `last`, normalized `length`, and byte `stride`.
Rank and element representation are properties of the resolved array type;
they are not additional runtime words. Dimensions are ordered outermost first.
Multidimensional arrays use contiguous row-major storage, with the final
index varying fastest. Arrays of arrays retain their component subtype shape.

| Role | Memory or call layout |
| --- | --- |
| Saved local/captured view or dynamic type bounds | data pointer, then one 32-byte dimension record per dimension; `8 + 32*rank` bytes. Type-bound storage does not use the data word. |
| Expanded unconstrained Ada parameter | data pointer, then four `l` arguments per dimension in first/last/length/stride order |
| Owned unconstrained result | data pointer, allocated transfer size, then dimension records; `16 + 32*rank` bytes. `AdaArrayResult` is the 48-byte rank-one header. |

`Value` carries bounds, optional materialized lengths/strides, and the data
operand. `arrayView`, `loadArrayView`, `storeArrayView`, `arrayRow`, `lengthOf`
and `arrayElementSize` centralize completion, marshalling and access. Element
addressing uses the dimension's stride; parameters, captures and result
consumers retain it. Static contiguous views can derive the same stride from
their type. No public foreign descriptor ABI, negative strides, transposed
arrays or arbitrary noncontiguous views are introduced. Bulk copies remain
contiguous and overlap-safe where assignment requires it.

## Checks and lifetime

- Bounds and slice/index operands are evaluated once and widened before use.
  Non-null bounds must satisfy index constraints. Null ranges preserve both
  endpoints and have length zero, including `Long_Integer'Last .. Long_Integer'First`.
- Length differences use unsigned/wide checked arithmetic. Counts beyond the
  signed 64-bit capacity, negative strides/sizes, byte-product overflow and
  allocation failure raise `Storage_Error`. Static impossible shapes receive
  diagnostics. Static record layout and activation-frame reservations use
  checked wide arithmetic too, so enclosing wide array components cannot wrap
  their offsets. Each stored suffix stride must itself fit, even for a null outer
  dimension. Dynamic type declarations validate dimension lengths/strides;
  total storage is checked when an object is allocated or copied.
- Indexing checks membership before calculating a checked byte offset. Invalid
  indexes and non-null slice endpoints raise `Constraint_Error`. A null slice
  keeps the base address without forming an out-of-object pointer.
- Conversion and parameter sliding check every dimension's length and relabel
  bounds without changing the source view. Even null ranges check that
  endpoints fit the target index base type; stream Input applies the same checks
  after adopting its buffer. Normal-return-only writable
  conversion copy-back and controlled component identity/lifetime rules remain.
- Owned results and local arrays use the existing per-activation arenas, caller
  adoption, construction/finalization owners and scope checkpoints. Null or
  zero-byte allocations reserve one byte. Allocation works while capturing an
  already-pending exception; helper success is not inferred from pending state.
- Large constrained local arrays, uncaptured record locals and scratch buffers (over 64 KiB) use managed
  allocation instead of oversized QBE stack allocations. Physical memory and
  host allocator/address-space limits still apply; wide descriptors do not
  promise that huge nonempty arrays can be allocated. Captured inline records
  are still bounded by QBE's signed 32-bit stack-frame representation; oversized
  frames are diagnosed (with alignment headroom) rather than wrapped. Large
  direct array locals use descriptors and managed storage instead.
- `'Length` and `'Size` use contextual integer checks before narrowing. Dynamic
  array object `'Size` is checked from its shape; runtime-constrained subtype
  `'Size` remains diagnosed. Bit sizes exceeding signed 64-bit result capacity
  are diagnosed or checked. Legacy runtime text/scalar APIs still take 32-bit
  lengths through an explicit checked bridge; they cannot silently truncate.

## Concatenation

Predefined `&` accepts nonlimited one-dimensional arrays in all four
array/element combinations, following
[RM 4.5.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-5-3.html).
Components may be scalars, access values, constrained arrays or records,
including nonlimited private and controlled components. Copies use the
component byte size, rather than assuming byte characters. Wide character
types and their literals remain Phase 7 work.

A null left array preserves the right operand's complete bounds, including
null endpoints. Otherwise an unconstrained ancestor preserves the left lower
bound; an ancestor defined with an index constraint uses that constraint's
lower bound. Element operands act as singleton arrays starting at the index
subtype's lower bound. Nonempty result upper bounds must belong to that index
subtype or `Constraint_Error` is raised. Count and byte-size capacity failures
raise `Storage_Error` (statically known storage overflow is diagnosed).

Operands are evaluated once per binary node. Nested concatenations retain
their individual bounds and exception checks. Each result has fresh temporary
storage; all components are copied before user `Adjust` callbacks run.
Controlled results use the shared construction/finalization owners, including
partial-adjustment failure and cleanup if evaluating a later operand fails.
Explicit user-defined operators continue to take precedence over predefined
homographs. Multidimensional, limited, task-containing and protected-containing
array operands are diagnosed rather than copied.

`concatenation` and `concattasking` exercise wide components and indexes,
nulls, constrained ancestors, dynamic bounds, generics, private components,
overload selection, all four profiles, component checks and controlled cleanup.
They run in whole/separate modes with deterministic-IR checks. `concaterrors`
checks rejected and ambiguous profiles; ACATS C45347A checks record components.
`runtime.concatenation` covers count/byte overflow and index endpoints in four
task contexts. Native and whole/separate Ada tasking runs use C runtime
ASan/UBSan and ThreadSanitizer builds; frontend tests also run under UBSan.
Generated QBE accesses remain uninstrumented.

## Compatibility and validation

This deliberately changes the internal Ada call/result ABI. Unit manifests use
format 6, unit records format 3; previous records are rejected and the changed
compiler digest invalidates cached units. Rebuild all compiler/runtime objects
and dependent Ada units together. Raw stream bounds are now two signed 64-bit
values; old serialized unconstrained-array streams must be regenerated. Raw
stream payload byte counts and element sizes are 64-bit. Broader portable stream
formats and large public I/O positioning remain separate library work.

Before widening, extracting the old layout contract preserved byte-for-byte IR
for ten representative array/conversion/capture/controlled/variant/I/O programs;
26 focused regressions passed. Frame allocation was audited in the emitter
(the semantic layer identifies captures, but does not allocate descriptor slots).
The resulting shared accessors cover locals, captured parameters/references,
result construction/consumption, aggregates, indexing/slicing, conversions,
comparisons, attributes/streaming and C runtime result helpers.

`arraydescriptors` and `arraydescriptortasking` cover wide positive/negative and
endpoint bounds, all-dimensional result/capture strides, runtime type bounds,
exactly-once bound evaluation, conversion copy-back, slices/nulls, checked Size,
managed large static locals, and lengths above Integer'Last using zero-size
elements. Both have whole/separate compilation and deterministic-IR checks.
`arraydescriptorerrors` pins count/byte/bit/record capacity diagnostics;
`arrayframeerrors` checks captured-object frame capacity.
`arraydescriptorstreams` round-trips wide text bounds and rejects those bounds
when the stream is read as String. `runtime.array_descriptors`
checks ABI sizes, endpoint arithmetic, products, offsets, ownership and pending
exception allocation in four task contexts. Existing result-buffer allocation
failure, stream truncation, controlled-result, conversion and lifetime tests
continue to pass. Native and whole/separate Ada tasking runs exercise C runtime
ASan/UBSan and ThreadSanitizer builds; generated QBE accesses remain uninstrumented.

## Explicit static Size

A static array Size clause can add trailing object storage. Descriptors still
measure contiguous component data; component sizes and byte strides include
any storage selected for the component subtype. Full objects and static result
buffers allocate the specified storage, while copies and array streams move
only component data. Padding follows an exact static shape and is dropped by
slice views and changed bounds. See [representation clauses](representation-clauses.md).
