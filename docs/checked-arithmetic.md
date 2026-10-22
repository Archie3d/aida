# Checked signed arithmetic

Signed integer operations lower directly from the semantic AST to QBE. The
arithmetic width is the base type's 32- or 64-bit class, independent of a
subtype's storage Size. Static folding and assignment/subtype checks retain
their existing contracts.

* Word add/subtract/multiply use exact signed-long intermediates and range
  checks before narrowing.
* Long add/subtract use the operands' and result's sign bits to detect
  overflow. QBE integer arithmetic wraps; no C signed-overflow assumptions or
  target-specific flags are required.
* Long multiplication takes a fast path when both operands fit signed words.
  Otherwise unsigned magnitudes and a quotient bound check the product before
  multiplication. Magnitudes can represent 2**63, so the minimum signed integer
  needs no overflowing absolute value. A zero multiplier permits every operand.
* Division checks zero and MIN/-1 before the native signed divide. Remainder
  substitutes divisor +1 for MIN/-1, producing zero without a hardware trap.
  Modulo adjusts a nonzero remainder to the divisor's sign, including MIN as
  divisor.
* Powers check negative exponents and use checked exponentiation by squaring.
  The final unnecessary square is omitted: MIN**1 remains valid, as do large
  exponents for bases 0, 1 and -1. 0**0 is 1. Operands are evaluated once.
* Negation and abs share checked subtraction.

Successful operations perform no integer runtime call or pending-exception
lookup. A failed check calls the existing raise routine and takes the normal
handler/propagation edge, preserving protected-action cleanup, task-local
exception state and assignment targets. The runtime integer helper remains
available for existing objects; no ABI or QBE backend change is needed.

Real, modular and scaled fixed-point operation helpers remain: they implement
finite-value, wrapping or rescaling arithmetic rather than these signed integer
checks. This optimization does not expand supported numeric ranges.

`checkedarithmetic` compares 88,000 cases per whole/separate run with an
independent C signed-128-bit oracle: endpoint and square-root boundaries,
zero/sign combinations, reproducible generated operands, powers and unary
operations. It checks exception identity and unchanged targets, and separately
checks operand evaluation counts. The C oracle also runs under ASan/UBSan;
generated machine code is not sanitizer-instrumented. `checkedtasking` checks
concurrent propagation and protected lock release after overflow. Both have
whole/separate determinism and tests forbidding integer-helper references in IL.
Existing overflow, sized-storage and exception tests remain in the full suite.

Unmodified ACATS C45504A and C45611A cover multiplication overflow and powers.
C45503A also passes unchanged, covering REM/MOD with named Left/Right
associations. See the [ACATS inventory](../tests/acats/README.md).

## Explicit operator associations

Predefined operator calls use Left/Right for binary operators and Right for
unary operators, as specified by [Ada 2005 RM 4.5(9)](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-5.html).
For example, `"rem" (Right => B, Left => A)` has the same operands as `A rem B`.
Named and mixed calls use the same candidate discovery as ordinary operator
expressions, filtering associations against each candidate's formal names before
applying overload preferences. User-defined operators retain their declared
names and homograph hiding; selection is preserved when the call is lowered.

Type discovery tries formal orders without moving, analyzing or evaluating the
actuals. Analysis commits one matching profile and reorders ownership once.
Predefined calls then use existing arithmetic/constant-folding paths; user calls
keep their selected binding. The existing expanded-name restrictions remain.

`namedoperators` checks reordered and mixed associations, case folding, unary
and binary numeric/Boolean/string operations, contextual overloads, generic
formal subprogram bindings, static expressions, evaluation counts and exception
behavior. `namedoperatorerrors` checks duplicates, positional-after-named calls,
unknown/missing/extra names, wrong operand types, hidden predefined profiles and
ambiguity. Whole/separate execution and deterministic IL are required.

The [benchmarks](../tests/benchmarks/README.md#checked-integer-arithmetic) compare
both normal and optimized runtime builds. They measure successful arithmetic,
not error-path throughput, and impose no timing threshold in CTest.
