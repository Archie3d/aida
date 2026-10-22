# Ordinary fixed point

The baseline is Ada 2005 RM [3.5.9](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-5-9.html),
[3.5.10](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-5-10.html),
[4.5.5](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-5-5.html)
and [A.5.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-A-5-4.html).

## Representation and rounding

Values use signed 64-bit counts with binary `Small = 2 ** (-bits)`.
Supported scale exponents are -62 through 62: Small may range from
`2.0 ** (-62)` to `2.0 ** 62`. The default is the largest supported power of
two no greater than delta. `System.Fine_Delta` is `2.0 ** (-62)`.
`Duration` retains its existing `2.0 ** (-30)` Small and time ABI.

An explicit `for T'Small use ...` clause selects another supported binary Small
no greater than delta. It must name a nonderived ordinary fixed-point first
subtype declared in the same region. The implementation currently requires the
clause in the representation-clause block adjacent to the type declaration,
before representation use; duplicates, late clauses, subtype/derived prefixes,
nonstatic values, nonbinary Smalls and self-reference through Small are
rejected. Expressions may refer to the type's Delta.

Static and dynamic count arithmetic share quotient/remainder scaling and
nearest rounding, with ties away from zero. Wide scale differences do not
cause temporary overflow when the result fits. Zero division and count overflow
raise Constraint_Error; static failures receive diagnostics. Exact scalar
constants retain the existing bounded evaluator: integer/rational magnitudes
are at most `2 ** 127 - 1`. This is not arbitrary-precision static evaluation.

Conversions to/from other numeric types rescale once, including unsigned
modular values above `2 ** 63 - 1` when the fixed result fits. Fixed_IO, Image
and Value support the wider binary scales using exact integer text conversion;
formatting does not approximate the stored value through a floating type.

## Operators and attributes

Fixed-by-fixed multiplication and division accept a numeric result type
identified by context or explicit conversion: ordinary/decimal fixed, signed integer,
modular or floating point. Operands may have different fixed types and scales.
The result is converted once, without first rounding to either operand's scale.
Floating results use the same exact rational-to-binary rounding as floating
Value, directly to binary32 or binary64. Integer results round ties away from
zero, then check the destination's base range.

The Integer scaling overloads use predefined Integer, as required by the RM;
other signed integer types require explicit conversion. A user-defined
primitive with two fixed parameters requires explicit conversion when selecting
the universal fixed operator, under the Ada 2005 rule. Ordinary calls to the
user operator remain available. `Standard."*"` and `Standard."/"` explicitly
select the predefined operation.

Small, Delta, Aft, Fore, First, Last, Base, Succ, Pred, Min, Max, Image, Value,
Size and Address remain available. Machine_Radix is 2; Machine_Rounds and
Machine_Overflows are True. Fore uses runtime bounds for dynamic subtypes;
Aft remains based on delta. Adjacent-value attributes use one count and the
base range, independently of a constrained prefix.

## Dynamic bounds and streaming

Named and anonymous fixed-point constraints may evaluate bounds at local or
library elaboration ([runtime scalar contract](runtime-scalars.md)). Bounds
are evaluated once into owning frame slots or library storage and checked
against their parent for nonnull ranges. Assignment, conversions,
qualifications, membership, calls, copy-back and returns share those bounds.
Fixed_IO instances can use these subtypes and capture their bounds.

Default fixed-point streams use eight native-order bytes containing the scaled
count, consistent with the existing representation-based streaming model.
Input checks the prefix subtype. Read checks its actual's subtype through a
temporary before copying back; an invalid count leaves the old object intact.
The stream advances even when that check fails. Dynamic fixed subtypes use
their runtime bounds. No new mutable runtime global state is introduced.

## Limits and evidence

Storage remains 64 bits; compact fixed-point Size clauses are still unsupported.
Only binary ordinary Smalls are supported, as permitted by RM 3.5.9.
[Decimal fixed point](decimal-fixed-point.md) has its own exact decimal scale.
Legacy delta constraints (J.3), complete freezing rules, portable
user-defined stream representations and Annex G guarantees remain outside this
stage. Unsupported syntax or representations are diagnosed.

`fixedwide`, `fixedwidechecks`, `fixedoperators`, `fixedoperatorerrors`,
`fixedwideerrors`, `fixedstreams` and `fixedtasking` complement the earlier fixed
regressions. Whole/separate execution and deterministic IR checks cover scales,
local bounds, operators, generic I/O, streams and four concurrent Ada tasks.
`runtime.fixed_arithmetic` exercises extreme scale differences, signed and
unsigned endpoints, exact floating rounding, text conversion, and independent
exception contexts on four threads. C helpers and generated programs are also
checked with AddressSanitizer/UndefinedBehaviorSanitizer; generated QBE accesses
are not instrumented. Unchanged upstream ACATS C35A08B covers mixed fixed
operands and explicit Standard operators. The
[coverage matrix](ada2005-coverage.md) and
[ACATS baseline](../tests/acats/README.md) record scope and provenance.
