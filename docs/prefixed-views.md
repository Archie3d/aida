# Bound prefixed views

Tagged prefixed subprogram views now work in subprogram renamings and as generic
subprogram actuals. Resolution matches the reduced profile (excluding the first
formal), including result type and parameter modes. Renaming parameter names and
defaults remain those of its own declaration; renamed views retain the actual
subtypes and passing modes. Renaming-as-body uses the existing completion checks.

The receiver is evaluated once when the renaming or instantiation is elaborated.
A stored reference or access value supplies the hidden first actual on subsequent
calls, so changing an access-valued prefix does not retarget the view. The ordinary
call analyzer supplies writable/aliased checks, implicit dereferencing/access,
overloads and class-wide dispatch before a forwarding body is constructed.
Internal operand names cannot hide names in the receiver expression.

Borrowed object receivers retain their identity. Owned tagged/controlled function
results use the enclosing master's result owner/arena, or library construction
storage for global views. Aggregate receivers and unmanaged enclosing array or
record results are materialized as constant objects with that master's lifetime.
Construction reuses ordinary object allocation, bounds/descriptors, runtime
record layouts and finalization; no new result ABI or mutable runtime state is
introduced.

The complete enclosing value is retained before selecting a component. Qualified
expressions, tagged conversions, selected components, indexing and slices refer into that
object; the selected component is not separately copied out. Array bounds and
receiver/index expressions are evaluated at elaboration, including dynamic and
unconstrained arrays and multidimensional aggregates. Subsequent calls reuse the
stored reference. Declaration-list temporary cleanup cannot reclaim this storage.
Normal exits, body exceptions, partial construction failure and a failed bounds
check while binding finalize already constructed captures and components.

Views have Intrinsic convention. Their renamings and forwarded generic formals
cannot be used to obtain an unbound callback via `Access`. Null access receivers
fail at elaboration. Full unreduced profiles, incompatible modes/results and
nonaliased access receivers are diagnosed. A temporary receiver remains a constant
view: materialization does not permit `out`/`in out` calls on it. The existing
restrictions on the enclosing types and expressions still apply, including the
Phase 9 restrictions on task/protected aggregate and result construction.

`prefixedviews` covers single evaluation, named/defaulted reduced parameters,
renaming-as-body, generic actuals, object/access dispatch, name capture, composite
results, null elaboration and a library-body view with a controlled receiver.
`prefixedviewcleanup` covers controlled function results, aggregate values and
components, normal/exception cleanup and partial elaboration failure. Both run
whole-program, separate-unit and deterministic checks. `prefixedviewlayouts` adds
array/runtime aggregates, conversions, slices, multidimensional indexing, unmanaged array/enclosing-record results, single evaluation and library
capture. `prefixedlayoutcleanup` checks runtime array/record aggregate components,
normal/exception cleanup, partial construction and failed index binding. These
fixtures also run whole-program, separate-unit and deterministic checks.
`prefixedviewerrors` pins profile, aliased-view, Intrinsic-access and constant-view
diagnostics; former layout rejections are now executable positive controls.
Unchanged ACATS C413005 passes alongside C413001–C413004; this is selected-test
evidence, not a complete conformance claim.

The language contracts are Ada 2005 RM
[4.1.3](https://www.adaic.org/resources/add_content/standards/05aarm/html/AA-4-1-3.html),
[6.4](https://www.adaic.org/resources/add_content/standards/05aarm/html/AA-6-4.html),
[8.5.4](https://www.adaic.org/resources/add_content/standards/05aarm/html/AA-8-5-4.html)
and [12.6](https://www.adaic.org/resources/add_content/standards/05aarm/html/AA-12-6.html).

## Temporary-layout validation

On Darwin arm64 (2026-11-09), all 1,978 native CTest checks passed on the final
compiler. All 25 focused prefixed-call/view and ACATS C413001–C413005 checks also
passed under ASan/UBSan and TSan, including the conversion and failed-construction
regressions. Commands used `ctest --test-dir build -j 8 --output-on-failure` and
`ctest --test-dir build/validation/<asan-ubsan|tsan> -j 2 --output-on-failure
-R 'prefixed|acats.c41300'`. These are local results; native Windows/Linux
validation remains with CI. Sanitizer evidence has the existing
[instrumentation limits](robustness.md).

## Historical validation evidence

Before the temporary-layout extension, the local Release suite had 1,968
distinct passing checks. Its final full run
passed 1,967/1,968: C651001 encountered a stale compiled-unit digest while the
compiler was being rebuilt; the affected case and the focused feature selection
were rerun after the final build. All 82 selected ASan/UBSan and 43 distinct selected TSan
checks passed, with the final receiver-layout cases rerun on each final compiler.
The [archived report](functional-views-validation.json) records each run, source
and binary hashes, the stale-build limit and ACATS provenance. Sanitizer coverage
retains the [existing instrumentation limits](robustness.md). No new profiling
measurements or native-host claims are included.
