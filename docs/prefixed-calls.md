# Tagged prefixed calls

Ada 2005 RM [4.1.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-1-3.html)
and [6.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-6-4.html)
define `Object.Operation (Arguments)` as a call whose receiver supplies the
first actual. AIDA supports this notation for the existing tagged-type subset:

- Specific and class-wide receivers, inherited and overridden primitives,
  class-wide operations declared in an ancestor's declaration region, and
  operations selected without a `use` clause.
- Explicit and implicit access dereferences, including access-to-constant;
  access-parameter operations receive the receiver's implicit `'Access`.
- Procedures and functions, omitted/defaulted arguments, named associations,
  overloaded receivers/operations, expected-result resolution and chained calls.
- Private tagged views retain operation visibility without exposing components.
  A visible component with the selector's name takes precedence over operations.

Resolution inspects the reduced profile before moving the receiver into the
ordinary call AST. The receiver occurs once. Existing call lowering supplies
dispatch, checks, default evaluation, by-reference passing, composite results,
exception handling and controlled-temporary cleanup. An implicit access actual
retains the receiver's class-wide view so controlling access calls dispatch.
The same rule now applies to explicit class-wide object `'Access` actuals.

Writable receivers require variable views. Access-parameter receivers also
require aliased views, including tagged formals and access dereferences; access
to a constant is accepted only for access-to-constant formals. Named access
first parameters, operations with a later tagged formal, untagged receivers,
invisible operations, malformed associations and ambiguous prefixes are rejected.
Null access receivers raise `Constraint_Error` through ordinary dereferencing.

`prefixedcalls` covers these paths, overloads, callable component precedence,
private tagged views, dispatching results, nulls and single evaluation, including
four concurrent workers and a library spec/body. `prefixedcleanup` checks
controlled receiver temporaries on success and exception propagation. Both run
in whole-program, per-unit and deterministic-IR modes. `prefixederrors` and
`prefixedviewerrors` pin rejection diagnostics. Unchanged ACATS C413001–C413004
pass the four combinations of value/access receivers and value/access formals;
they also cover nested package bodies in a library package body.

Interface receivers await Phase 6f. Prefixed-view renaming and generic
subprogram actuals are explicitly diagnosed and remain for the Phase 8 audit
(C413005). Taking `'Access` of a prefixed view is rejected because its Ada 2005
convention is intrinsic; it must not produce an unbound ordinary callback.
These view restrictions do not restrict ordinary prefixed calls.
