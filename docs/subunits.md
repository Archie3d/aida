# Separate subunits (Ada 2005 RM 10.1.3)

Subprogram and package body stubs (`is separate;`) are completed by
`separate (Parent)` bodies. The parent must be a library body or another
subunit. Stubs in specifications, blocks and ordinary nested bodies are
rejected. Stub identifiers must be simple and distinct within the parent;
missing, duplicate, orphaned and wrong-kind completions get diagnostics.
Subprogram declarations, stubs and bodies use the existing full-profile
conformance checks, including parameter names, modes, subtypes and defaults.
A package stub completes a declaration in the same declarative region.

A subunit sees declarations visible at its stub, including captured parameters
and locals, package private declarations and inherited context clauses. Its own
`with` and `use` clauses apply to that subunit and its descendants, without
leaking to the parent or siblings. Dependencies and elaboration pragmas from
subunit contexts participate in the parent's library dependency graph.
Limited context clauses on bodies remain illegal.

## Loading and artifacts

Automatic lookup uses the lower-case full parent/subunit name, with dots
replaced by dashes and an `.adb` extension. For example, `separate (Main.Work)`
`procedure Deep` is found in `main-work-deep.adb`. Explicit sources may have
custom filenames or bundle several units and subunits; registration order does
not determine lookup. Explicit subunits require a corresponding parent stub.

Bodies are expanded at their stubs before semantic analysis. Source locations
are retained, and generic token streams retain both expanded bodies and their
stub/context information for contract checking and each instantiation.
Subunits use the existing lexical frames, elaboration, masters and cleanup.
They do not declare library units or introduce independent binder items.

The parent object includes all transitive subunit sources in its manifest and
source digest. Changing a subunit invalidates that object; generic clients also
track the complete generic source digest. Compiling a subunit directly selects
its parent's object group, so the same dependency and ABI rules apply in
whole-program and per-unit compilation.

## Validation and remaining limits

`subunits` covers procedures, functions, defaults, package elaboration, private
parts, nested subunits, generic package bodies, context inheritance, captured
frames and four tasks. Whole-program/per-unit execution and deterministic IR
are checked. `semantics.subunits` covers loading, source order, direct subunit
compilation, incremental invalidation, illegal placements, profile mismatches,
context isolation and bad completions. `subprogramstuberrors` pins recovery
for illegal stub placements. Compiler AddressSanitizer checks cover positive
loading/generic paths and rejection scenarios.

Unchanged ACATS CA1006A, CA2009D and CA12002 execute successfully. CA2009A is an
exact compile XFAIL for the existing constrained-array actual restriction on
formal private types; its bodies are not edited to bypass that restriction.

Task and protected body subunits remain unsupported and diagnosed. Existing
restrictions on generic formals, types, tasking and profiles also apply inside
subunits. This implements the roadmap's subprogram/package subset, not all
four kinds of body stub in the RM.
