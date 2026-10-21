# Floating-point model and real edge cases

AIDA supports a finite-value floating-point subset using IEEE binary32 and
binary64. The language baseline is Ada 2005
[RM 3.5.7](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-5-7.html),
[4.5.5](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-5-5.html)
and [4.5.6](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-5-6.html).
This documents the implemented subset and its host requirements; it does not
claim complete floating-point attributes, Annex G accuracy or full conformance.

| Property | Float / types requesting 1–6 decimal digits | Long_Float / types requesting 7–15 digits |
| --- | --- | --- |
| Machine format / storage | IEEE binary32 / 32 bits | IEEE binary64 / 64 bits |
| Significand precision | 24 binary digits | 53 binary digits |
| Largest finite magnitude | `(2 - 2**(-23)) * 2**127` | `(2 - 2**(-52)) * 2**1023` |
| Smallest positive normal | `2**(-126)` | `2**(-1022)` |
| Smallest positive subnormal | `2**(-149)` | `2**(-1074)` |
| Default rounding | nearest, ties to even | nearest, ties to even |
| Machine_Overflows / Machine_Rounds | True / True | True / True |

The supported targets use native C `float` and `double` with these formats and
arithmetic at the declared precision. The runtime checks the format and
`FLT_EVAL_METHOD == 0` at build time. Fast-math/finite-only compilation is
rejected for the checked arithmetic implementation.

The floating environment must retain its platform defaults: nearest/even
rounding, gradual underflow and masked hardware exceptions. AIDA neither changes
rounding modes nor installs hardware-trap handlers. Foreign code must preserve
that environment, including in worker tasks. Other modes, flushing subnormals,
excess intermediate precision and a configurable floating environment are outside
this model.

## Operations and checks

Generated, nonstatic `+`, `-`, `*` and `/` call precision-specific runtime helpers.
They preserve binary32 or binary64 throughout the operation. A zero divisor of
either sign, a nonfinite input or a nonfinite result raises `Constraint_Error`
through the current task's context. In particular, `0.0 / 0.0` raises rather than
creating an Ada NaN value. Overflow is checked at each operation, even inside a
larger expression whose later operations would reduce the magnitude. Hardware
exception flags are not used as shared runtime state.

Underflow is allowed: a result can become a subnormal or round to zero without
an Ada exception. Precision diminishes in the subnormal interval; no claim is
made that it is a full-precision safe range. Basic zero signs can follow native
IEEE operations, but a language-level signed-zero guarantee and Signed_Zeros
attribute are not implemented; exact static zero does not retain a sign.

Float value boundaries now check the finite machine base range even for an
unconstrained subtype. Constrained subtypes retain their narrower range checks.
This includes the result of narrowing a dynamic Long_Float to Float: conversion
to infinity cannot silently succeed. Raw NaN/infinity representations supplied
by foreign code or byte-oriented facilities are outside the supported Ada value
model; arithmetic helpers and normal checked scalar boundaries reject them.
This is not a general validity check on every raw memory load or comparison.

`T'Machine_Overflows` and `T'Machine_Rounds` are static Boolean attributes for a
visible floating subtype mark, with no arguments. Both are True. Other missing
model attributes continue to receive unsupported-attribute diagnostics.

## Integer exponentiation of a real

The predefined operation is `Real ** Integer'Base`, distinct from the
real-exponent `"**"` exported by `Ada.Numerics` elementary-function instances.
The latter keeps its existing domain checks (including `Argument_Error` for
its zero-to-zero case and for negative bases).

For predefined real exponentiation:

- Positive, zero and negative exponents are supported, including
  `Integer'First`. The exponent must have type Integer; other specific integer
  types require an explicit conversion.
- An exponent of zero produces one, including `0.0 ** 0`.
- A negative exponent with either signed zero as the base raises
  `Constraint_Error`.
- The base and exponent are evaluated once. Exponentiation uses squaring, with
  at most 32 iterations for a dynamic Integer exponent. It widens the exponent
  before negating Integer'First.
- Negative exponents reciprocate the base before repeated multiplication. This
  avoids forming an overflowing positive power when the desired negative power
  merely underflows. Intermediate products are checked, and the unused final
  square is omitted, so `Real'Last ** 1` remains valid.

Products/reciprocals use the selected machine precision; no libm real-exponent
`pow` or conversion of the integer exponent to a floating value is involved.
This is the implemented association/rounding strategy, not an Annex G accuracy
promise.

## Floating Value

`S'Value (Text)` accepts a floating subtype mark and a String, returning
`S'Base` as specified by [Ada 2005 RM 3.5](https://adaic.org/resources/add_content/standards/05rm/html/RM-3-5.html).
The prefix's constraint is not checked by the attribute; assignment or
qualification to a constrained subtype still checks it. Floating subtype bounds
remain static; dynamic real constraints are not yet supported. Text is evaluated once,
including functions and slices with non-one lower bounds.

Accepted input includes leading/trailing spaces, an adjacent optional sign,
decimal or based numerals (bases 2 through 16, `#` or matching `:` delimiters),
underscores between digits, and decimal exponent numerals. Exponents scale by
the literal's base. Integer-looking forms and forms with digits on just one
side of the point are accepted too. Empty text, misplaced underscores, internal
spaces, format effectors, mismatched delimiters, invalid digits, trailing junk,
embedded NUL, C hex notation and NaN/infinity spellings raise `Constraint_Error`.

The parser accumulates an exact rational in allocated integer limbs and rounds
once to the destination binary precision, nearest/even, including subnormals.
It avoids double rounding for Float and does not depend on the C locale or
`strtod` grammar. Overflow raises `Constraint_Error`; underflow can round to
zero. Huge exponents are scanned completely and classified without huge
exponent loops; zero remains zero for any syntactically valid exponent.
There is no fixed-length text buffer or 127-bit static evaluator limit here;
allocation failure raises `Storage_Error`. Input length remains subject to the
current String descriptor's Integer limit. All scratch storage is per call and
exceptions use the current task context.

This is a separate attribute entry point from `Float_IO`'s existing
`__ada_real_value` scanner, whose token grammar, buffer and `Data_Error`
behavior remain a Phase 7 I/O audit item. Fixed-point parsing shares only the
integer-limb/digit primitives; its rounding and I/O contract are unchanged.

`realvalue` covers generic Float/Long_Float/derived types, constrained and null
prefixes, single evaluation, decimal/based syntax, malformed text and rounding.
`realvaluetasking` repeats the generic checks in four Ada tasks; both have
whole/separate and deterministic-IR checks. `realvalueerrors` pins prefix,
argument and arity diagnostics. `runtime.real_value` covers format endpoints,
ties, double-rounding traps, subnormals, all bases, long inputs and a decimal
conversion comparison with host `strtof`/`strtod`, in four native contexts.
Native and whole/separate Ada tasking checks pass with C runtime ASan/UBSan
and ThreadSanitizer instrumentation; generated QBE accesses remain uninstrumented.

## Static evaluation and evidence

Static expressions continue to use the
[bounded exact evaluator](static-arithmetic.md). Invalid evaluated static real
division and zero-to-negative powers are compile-time errors, while skipped
operands of static short-circuit forms remain exempt. Exact real powers now use
squaring too, removing the former exponent-magnitude-128 restriction; rational
capacity limits still apply. For example, some tiny or huge static powers are
rejected even though a dynamic operation fits the machine format or underflows
normally. Static checks are not replaced with unchecked host floating operations.

`realedges` runs generic checks for Float, Long_Float and a derived type:
negative/zero/positive powers, exponent endpoints, exactly-once operands,
subnormals, rounding to zero, signed zero divisors, immediate overflow,
subtype constraints and narrowing conversions. `realedgeerrors` pins static
failures, exponent type and attribute legality. `realtasking` runs the same
checks from four tasks, with independent exception handling. Both executable
programs have whole/separate compilation mode and IR determinism checks.

`runtime.real_arithmetic` exercises native helpers, nonfinite C inputs, extreme
exponents, precision differences and four thread-local exception contexts.
ACATS C45523A passes the declared Machine_Overflows policy for multiplication
and division by zero. Its sources/provenance are in the
[selected ACATS harness](../tests/acats/README.md).

ASan/UBSan and ThreadSanitizer checks instrument the C runtime, including whole
and separate builds of the tasking regression. Generated QBE memory accesses
remain uninstrumented; the Phase 8 validation gap is still open.
