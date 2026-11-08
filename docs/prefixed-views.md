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

Borrowed object receivers retain their identity. Function-result receivers use
the enclosing master's result owner/arena, or library construction storage for
global views. Static record aggregate values, including selected components, are
materialized as constant objects with that master’s lifetime. Declaration-list
cleanup therefore cannot prematurely finalize a retained receiver. Normal exits,
body exceptions and failed later receiver construction finalize existing captures.

Views have Intrinsic convention. Their renamings and forwarded generic formals
cannot be used to obtain an unbound callback via `Access`. Null access receivers
fail at elaboration. Full unreduced profiles, incompatible modes/results and
nonaliased access receivers are diagnosed. Binding aggregate receivers with array/runtime layouts and subobjects of
unmanaged array/enclosing-record function results remains explicitly diagnosed
and tracked as a follow-up. Object receivers and owned tagged/controlled results
use the supported paths.

`prefixedviews` covers single evaluation, named/defaulted reduced parameters,
renaming-as-body, generic actuals, object/access dispatch, name capture, composite
results, null elaboration and a library-body view with a controlled receiver.
`prefixedviewcleanup` covers controlled function results, aggregate values and
components, normal/exception cleanup and partial elaboration failure. Both run
whole-program, separate-unit and deterministic checks. `prefixedviewerrors` pins
profile, aliased-view, Intrinsic-access and unsupported-layout diagnostics.
Unchanged ACATS C413005 passes alongside C413001–C413004; this is selected-test
evidence, not a complete conformance claim.

The language contracts are Ada 2005 RM
[4.1.3](https://www.adaic.org/resources/add_content/standards/05aarm/html/AA-4-1-3.html),
[6.4](https://www.adaic.org/resources/add_content/standards/05aarm/html/AA-6-4.html),
[8.5.4](https://www.adaic.org/resources/add_content/standards/05aarm/html/AA-8-5-4.html)
and [12.6](https://www.adaic.org/resources/add_content/standards/05aarm/html/AA-12-6.html).

## Validation evidence

The local Release suite has 1,968 distinct passing checks. Its final full run
passed 1,967/1,968: C651001 encountered a stale compiled-unit digest while the
compiler was being rebuilt; the affected case and the focused feature selection
were rerun after the final build. All 82 selected ASan/UBSan and 43 distinct selected TSan
checks pass, with the final receiver-layout cases rerun on each final compiler.
The [archived report](functional-views-validation.json) records each run, source
and binary hashes, the stale-build limit and ACATS provenance. Sanitizer coverage
retains the [existing instrumentation limits](robustness.md). No new profiling
measurements or native-host claims are included.
