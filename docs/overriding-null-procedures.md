# Overriding indicators and null procedures

The Ada 2005 rules are [RM 8.3.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-8-3-1.html),
[RM 6.7](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-6-7.html),
and the dispatching and inherited-operation rules in
[RM 3.9.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-9-2.html)
and [RM 3.9.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-9-3.html).

## Indicators

Both indicators require a primitive operation. `overriding` must describe
an override already known at its declaration or completion; a hidden private
completion cannot justify an earlier assertion. `not overriding` also checks
operations acquired later, including null declarations and instances in private
parts. Bodies and renaming-as-body completions are checked against the completed
operation, independently of the indicator on its earlier declaration.

Subprogram instances retain their indicators and register primitives in the
instantiating scope after binding their profiles. Inherited and predefined
operator overrides are checked, including the implicit inequality belonging
to an equality instance. Contract replay distinguishes this synthetic operation
from the instance's own declaration and body. Instances also reject duplicate
homographs and require subtype-conformant dispatching overrides.

Protected operation and entry declarations accept `not overriding`; `overriding`
requires an implemented inherited operation, so it fails in the current
interface-free protected subset. Protected operation bodies cannot carry
indicators. Synchronized interfaces and their implementations remain Phase 9.

## Null procedures

A null procedure is a complete declaration. It can complete a fully conforming
ordinary procedure declaration, but cannot itself receive another completion.
Parameter names, modes, subtype constraints and repeated defaults retain the
ordinary full-conformance checks. A null declaration does not freeze preceding
types as a body would; tagged incomplete formal types can therefore be completed
later. Null declarations are not generic subprogram declarations/completions or
protected operation items.

Calling a null procedure still evaluates actuals and defaults and performs
parameter checks. Its entry skips the body-elaboration check: an earlier call,
including through a callback, does not require its declaration to have executed.
Inherited entries and dispatch use the existing call machinery.

## Controlling access results

Anonymous access results designating a specific tagged type can control dispatch.
Their designated tags follow controlling operands, enclosing dispatching calls,
or class-wide assignment through a dereference. Overload discovery and binding
preserve the dynamic designated view. A nominal overload-selection context on a
dereference does not impose a spurious access-value storage lifetime; genuine
storage contexts retain their accessibility checks and returned accessibility
words, including through dispatch and callbacks.

A controlling access result has no implicit null exclusion. Null is allowed
until a dereference or a non-null formal requires a check. Every concrete
descendant must override an inherited controlling-access-result function,
including null extensions; abstract descendants inherit an abstract obligation.
Anonymous access result profiles use designated-type conformance when matching
inherited overrides, bodies and renaming-as-body completions.

Controlling access parameters of renamed operations and subprogram instances
must already exclude null. All dynamic controlling operand tags are checked
before evaluating tag-indeterminate operands/defaults. Static inherited calls
continue forwarding their descendant tag to factory defaults. Returned objects
keep their identity and tag; an access result is never retagged like an owned
specific tagged value result.

## Validation and boundaries

`overridingnull` and `controllingaccessresults` exercise whole-program, per-unit
and deterministic compilation and four workers. They cover early calls and
callbacks, incomplete views, private completions, generic and renaming overrides,
operator instances, nullable results, nested factory dispatch, single evaluation,
exception propagation, accessibility and tag mismatch before default evaluation.
The `overriding*`, `null*`, `protected*indicator*`, `accessparameterlegalityerrors`
and `accessresult*errors`
fixtures check the corresponding illegal controls.

Private extensions and tagged discriminants now follow the
[tagged/private-view contract](tagged-discriminants.md). Interfaces retain their
remaining 6f owner. Generic abstract formals and null formal-procedure defaults remain
6g; null defaults receive an explicit unsupported diagnostic. General freezing
beyond the existing object/body/derivation checks remains in the Phase 8 audit.

Validation: 1,067/1,067 CTest cases and 30 focused compiler ASan/UBSan cases
pass. All 106 unchanged upstream ACATS baseline observations are retained,
including 56 executable passes; the recorded compiler fingerprint is updated.
