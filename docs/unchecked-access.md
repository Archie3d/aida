# Unchecked object access

`Object'Unchecked_Access` implements Ada 2005
[RM 13.10](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-10.html)
within the existing named and anonymous object-access subset. Context selects
the designated type, including overloaded access-returning prefixes and
controlling anonymous access parameters. There is no subprogram form in Ada 2005.

The attribute uses the same analysis and address lowering as object `'Access`,
but treats the designated object as library-level for accessibility. Static
object-depth rejection and runtime object/tag accessibility checks are omitted.
The library-level identity also travels through an anonymous access parameter,
so converting that parameter using ordinary `'Access` succeeds. Ordinary
`'Access` actuals retain their dynamic checks. Stand-alone anonymous objects and
components retain their own Ada accessibility rules after assignment.

All other existing rules remain: an aliased object view, a general access
context, compatible designated subtype and storage representation, and constant
protection are required. Unstable discriminant-dependent components are rejected.
Attribute arguments, type prefixes, function-value prefixes and subprogram
contexts produce diagnostics. The shared-storage escape restrictions still
apply. Explicit dereferences evaluate the prefix once and retain null checks.
Class-wide controlling arguments retain dispatch.

Unconstrained array bounds and mutable-record constrainedness use the existing
borrowed metadata; the attribute never owns the object's storage or extends
its lifetime. Normal finalization still runs at scope exit. Using the value
after the object ceases to exist remains erroneous under RM 13.10. Existing
full-type, representation and limited-record restrictions still apply.

`uncheckedaccess` exercises nested/library access types, object renaming,
constants, fields, aliased elements, explicit dereferences and overloaded
function results, generic instantiation, anonymous parameter forwarding,
stand-alone/component/discriminant contexts, multidimensional bounds, mutable
records, local tags, dispatch and controlled finalization. It runs directly and
with four workers in whole/per-unit modes, with deterministic IR checks.
`uncheckedaccesserrors` and `uncheckedsharederrors` record legality diagnostics, including an ordinary
`'Access` static-depth control. `uncheckedaccessir` and
`semantics.unchecked_access` check absence of object/tag accessibility helper
calls and continued null checks. Compiler AddressSanitizer checks cover these
compilation paths.

Unchanged ACATS CA110232 now executes successfully. CA120012 remains an exact
compile XFAIL for general limited records. Newly pinned C3A2004 is an exact
compile XFAIL for existing dereferenced subprogram `'Access` limitations, owned
by Phase 8; it is not an upstream execution pass. Its object `Unchecked_Access`
forms no longer produce diagnostics. Provenance is recorded in the
[ACATS inventory](../tests/acats/README.md).
