# Grouped generic formal objects

Phase 8 supports defining-identifier lists in generic formal object declarations,
including omitted or explicit `in`, `in out`, subtype marks, anonymous access
and null exclusions, and defaults supported by the existing formal-object
contract. Each name occupies its own position in positional/named association,
can independently use or override its default, and has its own instance symbol.

As required by [RM 3.3.1(7)](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-3-1.html),
a defining-identifier list expands in order into single declarations. The parser
retains the original declaration tokens for generic replay and produces fresh
subtype/default trees and single-object replay tokens for each formal. Identifier
locations remain distinct; formal-part use clauses retain their positions after
the expanded group. Existing object matching, lifetime and copy/reference rules
continue to apply.

Contract keys include the formal object's name while checking/replaying its
subtype and default. Source locations alone cannot distinguish copied defaults:
in `Seed, Other : Integer := Seed`, the first default can denote an outer Seed,
while the second denotes the preceding formal. Separate keys preserve those
bindings through nested generic replay and instantiation-site shadowing.
Default expressions are evaluated independently when needed. Grouped `in out`
formals retain each actual's identity, including repeated actuals.

The complete unchanged CC3601A scenario also needs predefined logical operators
on one-dimensional Boolean arrays. `not`, `and`, `or` and `xor` participate in
normal operator resolution and generic subprogram binding. Lowering evaluates
each operand once, pairs components by position, preserves the left operand's
bounds (the sole operand for `not`), checks binary lengths and returns independent
storage in the existing temporary arena. Null arrays, sliding bounds, nested
expressions, function results and exception cleanup use the normal array ABI.
Multidimensional/non-Boolean arrays and array short-circuit operators remain
illegal operands for these predefined logical operations.

`genericgrouped` covers independent/default/named/mixed associations, default
evaluation counts, declaration-site shadowing, nested template replay, package
and function instances, library generic bodies, formal-part use clauses,
unconstrained String actuals, null-excluding anonymous access, independent and
aliased `in out` actuals, and four workers repeating local instances.
`booleanarrayoperators` covers all four operators, generic callable actuals,
negative/different/null bounds, nested expressions, single operand evaluation,
returned arrays, length mismatch exceptions and four workers. Both run whole,
separately and deterministically. `genericgroupederrors`,
`genericgroupedduplicate`, `genericgroupedmodes` and `booleanarrayerrors` check
missing/duplicate/wrong actuals, variable requirements, duplicate identifiers,
illegal modes/defaults and invalid logical operand categories.

Unchanged CC3601A executes its full REPORT protocol as PASS. The 129-case
backfill observation outcomes remain unchanged: 92 PASS, 10 COMPILE_REJECTED,
one HARNESS_BLOCKED and 26 ungraded B-test rejections. CC3601A is registered
separately from that frozen selection.

Validation: the complete Release suite passes 1,765/1,765 checks. All 11 focused
checks pass with the ASan/UBSan compiler and instrumented C runtime, including
both local scenarios in whole/separate modes and unchanged CC3601A in whole
mode. The full 129-case snapshot refresh changes only the compiler hash;
upstream source hashes validate. QBE-generated Ada memory accesses remain
uninstrumented, and LeakSanitizer is disabled on this host.
