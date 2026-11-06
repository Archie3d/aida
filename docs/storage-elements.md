# System.Storage_Elements (Ada 2005)

The Pure child package implements the declarations and behavior of
[RM 13.7.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-7-1.html)
for AIDA's 64-bit, byte-addressed target model.

| Declaration | Choice |
| --- | --- |
| Storage_Offset | Distinct signed 64-bit integer type, System.Min_Int .. System.Max_Int |
| Storage_Count | Storage_Offset subtype 0 .. Storage_Offset'Last |
| Storage_Element | Distinct mod 256 type, Size 8 bits |
| Storage_Array | Unconstrained one-dimensional array indexed by Storage_Offset, with aliased byte elements and Component_Size 8 |
| Integer_Address | Distinct mod 2**64 type, with 64-bit storage |

Storage_Array components are contiguous bytes with alignment one. Existing
array-descriptor, object-size and allocation limits still apply; bounds are
separate from the byte payload. Slices and aliased components retain ordinary
array/access semantics, including bounds, null-dereference and lifetime checks.
No particular physical address is promised for an empty array or null slice.

## Address operations

To_Integer returns every pointer-representation bit as an unsigned value;
To_Address is its inverse. Null_Address maps to zero. All Integer_Address values
are accepted, including unaligned values and values outside mapped memory.
Conversions do not read memory, allocate, register ownership, check object
accessibility, or extend any object's lifetime.

Both Address + Storage_Offset overloads and Address - Storage_Offset operate
modulo 2**64. Negative offsets and Storage_Offset'First work without negating a
signed minimum. Address - Address first subtracts modulo 2**64, then interprets
the result as a signed two's-complement Storage_Offset; it never overflows.
These choices describe arithmetic on representations, including addresses of
different objects, rather than C pointer arithmetic within an array.

Address mod Storage_Offset treats the address bits as a nonnegative integer and
uses Ada's divisor-sign rule: a nonzero result has the sign of the divisor.
Storage_Offset'First and high-bit addresses are supported. A zero divisor raises
Program_Error, following the RM's advice for invalid address operations.
No other arithmetic/conversion operation raises an exception merely because
its address is unaligned, unmapped, or wraps. Later memory access still needs a
live object and the alignment required by the accessed type.

All seven public operations have intrinsic convention. Ordinary calls, expanded
and named operator calls, renamings and generic actuals/defaults work; subprogram
Access on an operation or its renaming is diagnosed. Native runtime entry points
implement the operations using integer arithmetic. There is no new mutable
runtime state; exceptions use the caller's existing task context.

## Compiler prerequisites and boundaries

System is now supplied by an analyzed [system.ads](../runtime/ada/system.ads).
Its children use ordinary parent dependencies and `system.spec` binding; no
System-specific missing-parent exception remains. The canonical Address type is
still bootstrapped, while the source supplies the public declarations and target
choices. Standard visibility remains available within the child.

The compiler accepts one static, confirming Component_Size clause on an
unfrozen, visible one-dimensional array first subtype with fixed-size components.
For Storage_Array this confirms eight bits; it does not change layout or
ownership. A derived array can have its own confirming clause. Queries support
array subtypes/objects, including implicit dereference, and evaluate an object
prefix once with normal exception behavior. Component_Size queries are
nonstatic under RM 4.9 even when the implementation knows the layout.

Nonconfirming clauses, duplicate/late/subtype clauses, multidimensional or
runtime-sized components, generic formal array/component layout queries, and
Pack after an explicit Component_Size clause retain diagnostics. Existing
packed Boolean arrays can confirm/query their one-bit component size. This is
not an expansion to general packing or stride-changing layouts.

## Validation and unchanged ACATS review

`storageelements` covers all declarations/profiles and operations, signed/unsigned
endpoints, carry/borrow and negative mod, zero-divisor and subtype failures,
renamings, generic actuals/defaults, slices, extreme indices, aliased byte
access, foreign memset/memcpy, empty-array address round trips, prefix evaluation
and null checks. The main task and four workers run independent checks, with a
protected success count. Whole/separate compilation and deterministic IR are
checked. `storageelementerrors` pins type/profile mismatches, intrinsic Access,
representation legality and unsupported-layout diagnostics, nonstatic query
use in a named number, and recovery after an undeclared array element type.

`runtime.storage_elements` checks 80,224 boundary/random pairs across four C
threads against widened mathematical reference arithmetic, including per-task
exception isolation and access to actual local byte storage.

The original 145-case backfill outcomes remain unchanged. Two additional
unchanged Ada 95 cases from pinned ACATS revision
`2785c30c1f8577d97ac7b3043882f6818781e9c5` were reviewed for Ada 2005 eligibility:

- CD30002 now resolves Storage_Elements and its To_Integer renaming. With an
  explicit local Impdef alignment adapter, it still rejects four object Alignment
  clauses and one scalar alignment requiring stride padding. It is not a pass
  or a target/linker-alignment validation.
- CD70001 now executes unchanged as PASS after
  [Address_To_Access_Conversions](address-conversions.md) supplies its previously
  missing child dependency; it joins the regular ACATS label.

Sources retain their original bytes and rights notices. The separate
[assessment manifest](../tests/acats/storage_elements/manifest.json) records
SHA-256/Git-blob hashes, exact diagnostics and outcomes. The
`baseline.storage_elements_acats` control checks remaining rejection observations
and source hashes; executable PASS cases are checked separately under ACATS. Re-run it after implementing the remaining
prerequisites rather than retaining obsolete rejection guards.

Final validation: Release 1,809/1,809 CTest checks pass. Nineteen distinct focused
ASan/UBSan checks pass, including the malformed-source campaign and the new
recovery regression. All 145 backfill observations remain unchanged; only the
compiler fingerprint changes. Generated Ada memory accesses remain uninstrumented
and LeakSanitizer is disabled on this host. Native Windows/MSYS2 and other target
execution retain the existing host-validation item.
