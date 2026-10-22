# Decimal fixed point

The target is Ada 2005 RM [3.5.9](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-5-9.html),
[3.5.10](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-5-10.html),
[4.5.5](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-5-5.html)
and [A.10.9](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-A-10-9.html).
This is selected decimal support, not full Information Systems Annex conformance.

## Types and representation

`type Money is delta 0.01 digits 12;` defines exact multiples of 0.01,
with an implicit first-subtype range of `-9999999999.99 .. 9999999999.99`.
Digits must be a positive static integer from 1 through 18. Delta must be an
exact static power of ten from `10.0**(-18)` through `10.0**18`. Small equals
delta. Optional explicit range bounds must lie within the mathematical digits
range before conversion; bounds are converted once to scaled counts.

Storage is signed 64-bit counts. The base subtype uses the full count range,
and its Digits attribute is 18. Derived types and subtypes retain the decimal
scale. The shared compiler/runtime scale ABI tags decimal exponents separately
from binary exponents; Duration's representation is unchanged. Decimal
Machine_Radix defaults to 10. A single adjacent Machine_Radix clause may request
2 or 10; either uses the same exact decimal Small and count arithmetic. Small
clauses remain available only for ordinary fixed types. Fixed Size is 64 bits.

Digits constraints retain the requested Digits value. Without an explicit range
they supply the implicit digits range; with a range they check both bounds
against the digits limit. Named local subtype declarations check parent
compatibility and digits limits at elaboration, including bounds computed at
runtime. Digits limits are checked even for null explicit ranges. Other static
digits constraints are supported when compatibility is provable; constraints
requiring an elaboration check outside a named local subtype are diagnosed.
Anonymous and library-level runtime scalar constraints remain unsupported.

## Arithmetic and rounding

Decimal multiplication and division truncate toward zero at the scale of the
specific result type, including Integer scaling. For example, a Money result
of `1.25 * 0.50` is 0.62, including `Money (A * B)` when that conversion
selects the fixed product's result type. Conversions of existing numeric values
round to nearest, with midpoints away from zero. `Money'Round (A * B)` instead rounds the mathematical
fixed product once: with A=1.25 and B=0.50 the result is 0.63. Round also accepts
real literals, real expressions and values of another fixed or floating type,
and returns the base subtype independently of the prefix's range.

Mixed decimal/ordinary fixed products and quotients use the existing numeric
result-context rules. Conversion to integer, modular or floating results does
not first round to an operand's scale. Conversion to binary32/64 rounds the
exact rational directly to the destination format. Conversion from binary
floating values uses the exact input binary value, including values close to
decimal midpoints. Overflow, zero division and range failures raise
Constraint_Error; static arithmetic failures receive diagnostics.

The compiler and runtime share exact integer arithmetic. Temporary precision
covers the supported scale differences without losing a representable result.
Static source rationals retain the existing checked 127-bit magnitude limit;
this does not provide arbitrary-precision source evaluation.

Digits, Scale and Round join the existing fixed attributes, including Small,
Delta, Aft, Fore, First, Last, Base, Succ, Pred, Min/Max, Image and Value. Scale
may be negative; Aft is at least one. Fore uses runtime subtype bounds.

## Generics, text and streams

Decimal formal types use `type Num is delta <> digits <>;` and accept decimal
actuals only. Ordinary `delta <>` formals accept ordinary fixed actuals only.
`Ada.Text_IO.Decimal_IO` supplies file, current-file and string Get/Put with
standard formatting defaults. It shares exact count-based text conversion
with Fixed_IO: decimal/based input, exponent syntax, field widths, delimiter
handling, midpoint rounding, Data_Error and Layout_Error checks do not pass
stored values through binary floating point. Image and Value share that model.

Default streams use eight native-order count bytes. Input checks the prefix
subtype; Read checks the actual subtype before copy-back and leaves the target
unchanged on failure. This is the existing representation-based stream model.
The numeric helpers have call-local scratch; text ownership and I/O locking
follow the existing runtime and introduce no mutable global state.

## Evidence and remaining work

`decimalpoint`, `decimalchecks`, `decimalerrors`, `decimalstreams` and
`decimaltasking` cover static/runtime arithmetic, rounding, digits constraints,
conversions, formals, text, streams and four concurrent Ada tasks. Execution in
whole/separate modes and deterministic IR checks accompany these cases.
`runtime.decimal` checks extreme scales, signed/unsigned endpoints, exact float
conversion and text with four independent C task contexts. The C runtime and
generated programs are also checked with AddressSanitizer/UndefinedBehaviorSanitizer;
generated QBE memory accesses are not instrumented.

Unchanged upstream ACATS CXF2002 and CXF2003 exercise repeated multiplication,
division, truncation and Round with matching/different decimal operand types,
for both requested machine radices. See the [ACATS baseline](../tests/acats/README.md)
and [coverage matrix](ada2005-coverage.md).

Legacy delta constraints, additional A.5.4 model attributes, full freezing,
compact storage, Ada.Decimal.Divide and the rest of Annex F remain future work.
Ordinary fixed Small is still restricted to powers of two; ACATS cases requiring
ordinary decimal Small are outside this selection. Unsupported constructs or
representations receive diagnostics.
