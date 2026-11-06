# Standard and System (Ada 2005)

The compiler supplies Standard.ASCII ([RM J.5](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-J-5.html))
and the public declarations of System ([RM 13.7](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-7.html)).
ASCII is nested in Standard and directly visible without a with clause. All
79 constants have type Character and their prescribed values: controls,
punctuation and lower-case letters. Package/object renaming, static use and
address-taking retain the same immutable objects across units and tasks.

## Target choices

All supported QBE targets use the same 64-bit, little-endian data model.
System.Name has one literal, AIDA_64; System_Name is that value. This identifies
the shared data model rather than the host OS or instruction set.

| Declaration | Value or representation |
| --- | --- |
| Min_Int / Max_Int | -2**63 / 2**63-1, the supported signed type-definition bounds |
| Max_Binary_Modulus / Max_Nonbinary_Modulus | 2**64 / 2**64-1 |
| Max_Base_Digits / Max_Digits | 15, the binary64 precision selection limit |
| Max_Mantissa | 63, the signed 64-bit ordinary fixed-point magnitude precision |
| Fine_Delta | 2**(-62) |
| Tick | 1/1_000_000 seconds, universal_real |
| Address / Null_Address | Existing 64-bit address identity / zero pointer |
| Storage_Unit / Word_Size | 8 / 64 bits |
| Memory_Size | 2**64 storage elements, the model's theoretical address space, not installed RAM |
| Bit_Order | (High_Order_First, Low_Order_First) |
| Default_Bit_Order | Low_Order_First, a static constant |
| Any_Priority | Integer subtype 0 .. 31 |
| Priority / Interrupt_Priority | Any_Priority subtypes 0 .. 30 / 31 .. 31 |
| Default_Priority | 15, (Priority'First + Priority'Last)/2 |

Except Fine_Delta and Tick, numeric limits are universal-integer named numbers;
Memory_Size and the maximum moduli can exceed every signed machine integer.
Name, bit-order and priority constants are typed static constants with immutable
runtime storage. Null_Address is a nonstatic constant with immutable storage.
No declaration allocates task-owned state or changes object lifetimes.

Address comparisons use unsigned pointer-bit order, with equality comparing
identity. The six Left/Right profiles return Boolean; package selection,
renaming and generic actuals/defaults use the existing intrinsic operator
machinery. As with other intrinsic operations, taking a renamed comparison's
Access is diagnosed. Ordering does not imply that two different objects belong
to one array, nor does it grant dereference or ownership rights.

Priority declarations describe representable values; Phase 9 still owns
priority scheduling. Bit_Order declarations do not extend supported
representation clauses to nondefault bit order. Existing address extensions,
including acceptance of null literals and directly visible address comparisons
without a use-type clause, retain their previous behavior.

## Evidence

`standardfacilities` checks all ASCII values, static limits, enum attributes,
priority subtype bounds and a dynamic range failure, constant addresses and
renamings and cross-unit identity, related address conversions without ownership,
unsigned high-bit ordering, all six comparison renamings, selected/named calls, generic actuals,
a named generic default and selected lookup under a local operator homograph.
The environment task and four workers run independent checks; whole/separate
compilation and deterministic IR are checked. `standardfacilityerrors` rejects
wrong result/mode profiles, an incompatible generic actual, intrinsic Access,
unrelated address/access conversions, type mixing and writes to constants. The inventory removes the 18 obsolete
Standard/System API-gap guards; Stream_IO's four API-gap guards remain.

Unchanged C54A03A and C64005C now execute as PASS. C35502C compiles and runs,
but fails its leading/trailing HT rejection controls for enumeration Value.
That exposed scalar-attribute behavior remains a P3/P8 correctness gap, not an
ASCII implementation failure or a conformance pass. The remaining 142 backfill
outcomes are unchanged. Upstream sources and hashes remain untouched.

The complete Release suite passes 1,803/1,803 CTest checks. Thirteen distinct
focused ASan/UBSan checks pass, with the final expanded four-test facility
subset repeated successfully. Generated Ada accesses remain uninstrumented;
LeakSanitizer is disabled on this host. Other host/target execution retains
the roadmap's host-validation item.
