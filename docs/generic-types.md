# Generic type formals — Phase 6g, first increment

The target is Ada 2005 RM [12.5.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-12-5-1.html)
and [12.5.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-12-5-4.html).
This increment extends the existing numeric, discrete, array and private
formals; it does not complete Phase 6g.

## Private actual subtypes

A definite private formal accepts constrained array subtypes, including runtime
bounds and multiple dimensions. Matching examines the actual subtype rather
than stripping its constraint through the base-type identity. Instance bindings
retain that subtype, so object initialization, copies, reference actuals, results
and sliding use the existing descriptor and ownership mechanisms.

`type T (<>) is private` and `type T (<>) is limited private` allow indefinite
actual subtypes. The checked contract retains the unknown-discriminant view:
uninitialized variable objects, array/record components and passing that view
to a definite formal are diagnosed before any instance is needed. A private
formal without discriminants also rejects an unconstrained array, a class-wide
subtype or a discriminated record lacking both constraints and defaults.
Limited actuals still require a limited formal.

## Access formals

Named object access formals support pool-specific, general variable and constant
access, and null exclusions. Named access-to-procedure/function formals support
the existing callback profiles. Matching enforces:

- Object versus subprogram access and statically matching designated subtypes.
- Constant access on both sides or neither; an `all` formal requires general
  variable access. A pool-specific formal can accept general variable access.
- Identical outer null exclusions.
- Mode-conformant designated callback profiles: parameter count, modes, types
  and result type. Parameter names and scalar subtype bounds may differ.

Each formal access definition is checked as a named type in the generic's
contract scope. A fresh definition is resolved for matching with preceding
formals bound to actuals and declaration-site names replayed from the contract.
These matching-only declarations stay owned by the instance but are not emitted
as new access collections. The instance binds the original actual type, reusing
its allocation collection, accessibility, callback convention and checks.
Contract violations such as writing through access-to-constant, arithmetic on a
private designated object or an invalid callback call are rejected even without
an instantiation.

## Evidence and remaining work

`genericaccess`, `genericaccessmatching` and `genericprivatearrays` run in whole,
per-unit and deterministic modes, including four concurrent workers.
`genericaccesserrors`, `genericaccesscontracterrors`, `genericaccesssyntaxerrors`
and `genericprivateerrors` pin legality diagnostics with legal controls.
The unchanged ACATS CA2009A, CC54001, CC54002 and CC54003 execute successfully;
provenance and deferred cross-feature cases are in the
[ACATS inventory](../tests/acats/README.md#generic-private-and-access-types-phase-6g-first-increment).
The full suite passes 1,154/1,154 tests. Thirteen focused compiler ASan/UBSan
checks and all three execution scenarios in whole/per-unit modes with the
instrumented C runtime pass. LeakSanitizer is unavailable on this host;
QBE-generated Ada memory accesses remain uninstrumented. All 123 recorded
ACATS baseline observations are unchanged.

Known discriminants on private formals, tagged/abstract private formals,
derived/interface formals, Definite, formal packages and the later 6g bullets
remain open. Named protected callback types retain the existing Phase 9
restriction. Generic access objects and limited `in` objects remain diagnosed.
The predefined Unchecked_Deallocation compatibility contract is unchanged.
