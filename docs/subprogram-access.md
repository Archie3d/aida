# Imported and inherited subprogram access

Phase 6d supports `Access` of Ada imports, C imports with a matching named
C callback type, and inherited tagged procedures/functions. Ordinary callback
profile checks, null exclusions, accessibility, renaming identity and exception
propagation still apply. Intrinsic subprograms, including inherited untagged
operations, remain illegal prefixes.

## Dereferenced callbacks

`Callback.all'Access` and `Function_Returning_Callback.all'Access` are supported
as a Phase 6e follow-up. The expected designated profile selects an overloaded
prefix. The compiler evaluates the access-valued prefix once and checks it for
null immediately, raising Constraint_Error even if the resulting callback is
never called. It reuses the existing callback value, preserving its code,
captured environment and, for protected callbacks, protected receiver. Native
C callbacks keep their native pointer representation.

Profile subtype conformance, parameter modes, calling convention, protected
convention and private-view visibility remain checked. Under RM 3.10.2(15),
the dereferenced subprogram has the accessibility of the source access type.
A deeper named type cannot escape through this attribute; a derived access type
retains its ancestor's level. Anonymous callback parameters can be forwarded to
other anonymous callback parameters, but cannot escape into named or stand-alone
anonymous callback objects. `Unchecked_Access` remains object-only in Ada 2005.

`callbackdereference` covers captured variables, procedure/function callbacks,
record/array components, overloaded parameterless and explicit function calls,
forwarding, protected and C callbacks, nested access types, null checks and
failed-assignment preservation. Whole/per-unit execution, deterministic IR and
four workers are checked. `callbackdereferenceerrors` checks profile/mode/subtype,
convention, privacy, ambiguity and accessibility diagnostics. Compiler ASan
covers both paths. Unchanged ACATS C3A2004 now compiles and executes successfully.

## Imported subprograms

Ada imports use the AIDA Ada calling convention, including array descriptors,
hidden result/ownership arguments and anonymous-access metadata. Both direct
and indirect calls use that convention. The imported body must use the same
AIDA ABI; this does not establish compatibility with another Ada compiler.
Imported controlled results retain their existing diagnostic.

A named access-to-subprogram type can specify `pragma Convention (C, Type_Name)`
(or explicitly `Ada`). Specify it once, before the type is used in a subtype,
component, object or subprogram profile. The pragma currently supports named
callback types only. C callback profiles accept scalar and named access
parameters/results; composite and anonymous-access profiles are diagnosed.

C callback values are native function pointers. They can be called from Ada,
passed to an imported C routine, or returned by one. Indirect calls reuse the
existing C argument/result marshaller, including floating point classes,
representation-coded enumerations, writable scalar parameters and null checks.
Function addresses are relocated through constant data, which also works for
external symbols on macOS without modifying QBE.

The subprogram and access type must have matching conventions. An ordinary
Ada callback or anonymous Ada access definition cannot directly designate a
C import. Defining ordinary Ada subprograms with convention C, exporting Ada
callbacks to C, general foreign composite profiles and broader ABI portability
remain later interfacing work and receive diagnostics where unsupported.

## Inherited tagged operations

Each inherited tagged operation has an entry point with its inherited profile.
It invokes the ancestor implementation without dispatching and uses the
existing argument/result paths. Inherited constructors retag their result for
the derived type. Controlling access parameters retain their null exclusion
and accessibility metadata. Renamings designate the same callback entry.

Library units emit stable entries/descriptors even when their own source does
not take `Access`: a separately compiled consumer must be able to take it
without recompiling the defining unit. Nested entries capture the master of
the derived type's declaration, preserving outer variables and dynamic tags.
Packages do not add accessibility levels. Invoking an entry still checks the
ancestor body's elaboration flag, so taking a callback before that body's
elaboration does not bypass `Program_Error`.

The existing restrictions on private extensions, limited/tagged definitions,
generic formal access types and named protected callback types retain their
Phase 6f/6g/9 owners. Completing Phase 6d does not claim those features.

## Validation

`importedcallbacks`, `inheritedcallbacks`, `inheritedcallbacklibrary` and
`callbackextensiontasking` run whole/separate compilation and determinism
checks. They cover Ada imports with strings/results and exceptions, captured
nested operations, writable formals, inherited constructors/tags, renamings,
identity, calls before/after ancestor-body elaboration and four concurrent
workers. `runtime.importedcallbackabi_modes`
links a C helper to prove native pointer forwarding/returns, float/double
arguments and writable scalar copy-back in both compilation modes.
`callbackextensionerrors` checks convention mismatch, unsupported C profiles,
late representation changes and late Import pragmas.

Unchanged ACATS C3A0006 passes for callback values stored in arrays and
invoked through a library package. C3A0009 exercises inherited operations but
remains a pinned compile XFAIL for private extensions (6f) and package
renaming (6e); none of its runtime assertions are weakened or claimed as run.
The selected C interfacing ACATS cases also require the later Interfaces.C
library and full foreign-profile support.

Normative references: [RM 3.10.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-10-2.html),
[RM 6.3.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-6-3-1.html),
[RM B.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-B-1.html).
