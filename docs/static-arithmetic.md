# Static arithmetic and implementation limits

AIDA evaluates supported static scalar arithmetic during semantic analysis,
separately from checked machine arithmetic in generated programs. The baseline
is [Ada 2005 RM 4.9](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-9.html),
especially compile-time evaluation, checks, maximal static expressions and final
rounding in paragraphs 33–38. This remains a bounded subset, not arbitrary
precision or complete RM 4.9 conformance.

Integer literals and named numbers retain an exact value independently of the
32/64-bit machine-value cache. Named integer numbers retain universal_integer,
including across library-unit boundaries. Unary and binary integer arithmetic,
comparisons, Boolean operations and supported discrete membership tests fold
without intermediate machine overflow checks. Integer division truncates toward
zero; rem follows the dividend and mod follows the divisor. Modular operations
continue to use their modular semantics.

The completed expression is checked against the base range of its expected
specific integer type. A subtype constraint on ordinary assignment still runs
at runtime; this distinction preserves the C52005A assignment tests. Explicit
static conversions, qualifications and scalar attribute operations must pass
their own checks, including subtype constraints, even inside a larger static
expression. Static divide/mod/rem by zero and negative integer exponents are
diagnosed. The evaluated portion of a static short-circuit form excludes its
skipped right operand; ordinary eager Boolean operators have no exemption.
A dynamic right operand prevents the form from being static.

Static real arithmetic retains reduced rational values until a machine result
is needed. Final conversion generates binary significand bits from the fraction
and rounds once, nearest with ties to even, for the supported 24-bit Float and
53-bit Long_Float representations. It does not first approximate the fraction
as a double and then round it again to Float. Explicit real conversions and
references to typed floating constants respect the machine value at that
boundary; universal real named numbers keep their exact fraction. Fixed-point
rounding remains nearest with ties away from zero through the existing scaled
count helpers. The broader floating-point model/runtime edge-case audit remains
the next Phase 5 item.

## Bounded evaluator

These are explicit implementation restrictions, not Ada range rules:

- Integer magnitudes and rational numerator magnitudes are at most
  `2**127 - 1`; rational denominators are positive and at most that value.
  The most negative signed 128-bit value is excluded to make normalization
  and negation safe. The most negative signed 64-bit literal is supported in
  decimal and based notation.
- Every checked intermediate in rational normalization/arithmetic must fit
  that capacity. An operation can therefore hit the limit even when a later
  cancellation or reduction would make the final result small. Fractions are
  compared by continued quotients, avoiding overflowing cross-products.
- Nonzero literal radix exponents and exact real power exponents are limited
  to magnitude 128. Based-literal exponents and decimal integer exponents must also
  fit signed 64 bits in the lexer, including zero-valued literals; integer
  literal exponents must be nonnegative. Zero with
  a representable large exponent is handled without iterating over it.
  Integer expression powers use checked exponentiation by squaring, bounded
  by result capacity rather than a linear loop over the exponent.
- Integer type bounds, runtime values and fixed-point scaled counts remain
  limited to 64-bit storage. Static fixed-point operations also require each
  scaled intermediate to fit signed 64 bits; otherwise they produce a named
  evaluation-limit diagnostic rather than falling back to runtime arithmetic.
- Static operations on typed binary floating constants may require an exact
  binary fraction outside rational capacity, even when the stored machine
  number is supported. These operations are diagnosed as evaluation-limit
  failures. Comparisons between machine constants, or between an extreme
  machine constant and a bounded rational whose ordering is provable from
  their magnitudes, remain supported.

Exceeding these limits produces a diagnostic; it never substitutes a wrapped,
clamped or zero machine value. `Ada.Numerics.Pi` and `e` use 35 fractional decimal
digits, fitting the exact evaluator while exceeding the precision of the
supported machine real types. Previously their longer source literals could
silently fall back to host-double approximation. Runtime numeric I/O still
parses through its own checked conversion routines; this decision does not
reduce the runtime floating-point range.

Static string evaluation, complete generic-formal staticness rules and the
remaining scalar attributes still require the broader Phase 8 gap audit. This
change extends the existing supported scalar evaluator; it does not claim
that every syntactic form listed in RM 4.9 is implemented.

## Evidence

`staticarithmetic` exercises literal endpoints, large integer cancellation,
integer signs/mod/rem, exact real cancellation, single rounding around Float and
Long_Float ties, typed floating constants, skipped checks and preserved dynamic
overflow. `static_numbers.ads` supplies exact named numbers from another unit;
`determinism.staticarithmetic` compares whole and separate compilation IR.
`staticarithmeticerrors` covers invalid static checks, machine boundaries,
conversion/qualification/attribute failures, nonstatic short-circuit forms and
integer/real/fixed evaluation limits. `integerliteralerrors` and `baseerrors`
cover lexical capacity/exponent failures. Existing runtime check regressions
use variable operands where a static failure is now a compile-time error.

The unchanged upstream ACATS C49022A case covers integer named-number
arithmetic and comparisons. It runs through the
[selected CTest harness](../tests/acats/README.md); see the
[coverage matrix](ada2005-coverage.md) for restrictions and roadmap ownership.
