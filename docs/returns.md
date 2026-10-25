# Extended returns and limited construction

Ada 2005 RM 6.5 and 7.5 govern the implemented return and noncopying rules.

## Extended return statements

`return Result : [aliased] Subtype [:= Initializer] [do ... end return];`
works for scalar, named/anonymous access, array, record, tagged and class-wide
result types within the compiler's existing type and layout subset. `constant`
is rejected: it is not part of the Ada 2005 return-object syntax.

The nominal return subtype is elaborated once. Constrained result subtypes
must statically match it; indefinite array/class-wide objects require an
initializer. Named subtype indications and anonymous access definitions cannot
be interchanged. Initialization precedes the handled sequence, so its handlers
cannot catch initialization failures. The return object is visible only in its
sequence and handlers; `return;` completes the nearest extended return, whereas
nested subprogram returns apply to those subprograms. A value-bearing return
inside the extended return is illegal.

Ordinary scalar and nonlimited untagged composite objects use existing local
object initialization and result copying. Owned/tagged and limited results
reuse caller-owned construction. Goto, exit and exceptions can abandon an
extended return without completing the function. Completed components and
coextensions are cleaned up before the next return attempt. Local masters are
left before completing a return, including waiting for local tasks.

A noncontrolled dynamic-array return uses the existing transfer-buffer ABI.
Its descriptor starts null; if finalization or master cleanup raises while
leaving the function, `__ada_array_result_cleanup` frees any untransferred
buffer and preserves the pending exception. Successful results are adopted by
the caller as before.

## Limited full views and build in place

Explicitly limited record definitions have been pulled forward from Phase 6f
as a prerequisite. Intrinsic full-view limitedness is tracked separately from
a limited private view with a nonlimited completion. It follows subtypes,
private completion and derivation, and propagates through composite parts.
Tagged records containing limited parts require explicit `limited`.

Assignments, initialization from existing limited objects, predefined equality,
and copying limited component/ancestor expressions are rejected. Limited
initializers and returns must be aggregates or function calls (including
operator notation), optionally
qualified/parenthesized. Component defaults obey the same rules. A limited
private type with a nonlimited full view retains its existing internal copying
semantics.

Limited records and arrays construct directly into their final destination,
including grouped/nested aggregates, component constructors, forwarded calls,
extended-return initializers, allocators, unconstrained results and tagged/
class-wide results. Constraints are checked before successful completion;
self-references retain their identity. Owned results use the existing
result pointer/descriptor, owner, arena and destination-provided parameters.
No new runtime ownership registry or global mutable state is introduced.

## Validation and boundaries

`extendedreturns`, `limitedresults` and `returncleanup` run whole-program,
per-unit and deterministic checks. They cover scalars, arrays/matrices,
tagged/class-wide objects, generics, aliased objects, captured frames, four
workers, self-reference identity, constrained result failures, allocation,
local task masters, partial initialization, exception handlers, abandoned
returns and failing finalizers. `semantics.returns` pins legality diagnostics.
`runtime.array_storage` counts actual transfer-buffer frees on success/failure.
Compiler AddressSanitizer checks cover these compilation/rejection paths.
C runtime ASan/UBSan checks cover the allocation-counting unit test and the
three executable fixtures in whole/per-unit modes. Generated QBE memory
accesses are not instrumented.

Unchanged Ada 2005 ACATS C650003 passes limited/generic extended returns and
simple return/exit/goto/exception transfers. CA120012 now passes its limited
record declarations unchanged. C3A0025 remains a pinned generic-access-object
compile XFAIL; its former limited-record diagnostic is removed.

Task/protected aggregates and function results remain explicitly diagnosed.
Phase 9 must add caller-master adoption and delayed activation for task-valued
results, and the corresponding protected lifecycle work. Phase 6f retains
private extensions, tagged discriminants, runtime-sized tagged layouts and
limited current-instance access in defaults. Phase 6g retains limited generic
formal-object initialization and the remaining formal-type categories.
The existing unsupported type/layout restrictions also apply here.

Normative references: [RM 6.5](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-6-5.html),
[RM 7.5](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-7-5.html).
