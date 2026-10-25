# Anonymous access definitions

Phase 6d accepts anonymous access definitions for stand-alone objects and
renamings, record/array components, record discriminants, parameters, function
results and anonymous extended returns. Access-to-constant and anonymous
access-to-subprogram profiles are supported, including nested callbacks and
protected procedure/function callbacks. Stand-alone arrays may define their
array type inline, for example `A : array (1 .. 10) of access Cell :=
(others => new Cell);`. Each grouped object gets its own array type, bound
evaluation and initializer. These declarations reuse the named-array layout,
collection and initialization paths. Access parameters have no explicit
parameter mode. Their profiles participate in overload resolution, declaration/
body conformance and tagged overriding. Controlling access parameters dispatch
through the designated object's tag and implicitly exclude null.

This extends the [named general-access contract](general-access.md).
[Null exclusions](null-exclusions.md) cover named and anonymous definitions.
[Imported and inherited tagged callbacks](subprogram-access.md) extend the
same callback and call machinery. The existing restrictions on
limited/tagged definitions (6f), generic formal objects/types (6g), and task/
protected discriminants and named protected callback types (9) still apply and
produce diagnostics. Anonymous access definitions require subtype marks;
constraints belong on a named designated subtype. They cannot specify `all`.

## Accessibility and ABI

Access parameters carry the actual's accessibility identity through direct and
indirect calls, forwarding, object renaming, dereferences and conversions.
Checking lexical depth alone is insufficient: a callee's local access type can
safely designate an object in a caller's deeper lexical block. Runtime checks
therefore use master ancestry, including task dependence; siblings are
incomparable. Statically illegal escapes still receive diagnostics. Failed
runtime accessibility checks raise Program_Error before assignment/copy-back.

The runtime lazily assigns non-reused integer identities to live masters.
The shared index is locked, and a master's record is removed on leave. Library
lifetime is identity zero. Task dependence carries identities rather than
pointers into another task's stack. Collections snapshot their declaring
master's identity. Aliased object views snapshot their object's identity;
renamings inherit it. Derived named access types retain their ancestor's pool
and accessibility. Records containing access discriminants are checked at
returns and allocation boundaries.

Ordinary object access values retain their eight-byte pointer representation.
Anonymous object parameters additionally store an accessibility identity in a
16-byte slot. Access discriminants use 24 bytes: pointer, identity and a
coextension owner. Discriminant-constraint scratch slots use 32-byte strides.
Ada parameter calls pass hidden accessibility words. Anonymous access results
pass a hidden result-level address, including through callbacks, so a callback's
actual result lifetime cannot be confused with its formal profile's lifetime.
Foreign C object-access arguments retain pointer arguments; the wider foreign
ownership/convention contract remains Phase 8 work.

Nested ordinary callback descriptors retain code/static-link pairs. Protected
callback descriptors also carry their receiver and use normal protected action
entry, completion and exception handling. Their storage belongs to the access
definition's collection (or the call's temporary collection for parameters).
Their receiver's lifetime is checked before a stored descriptor is published.

Unconstrained arrays retain the shared borrowed-descriptor index. Conversion
copies required metadata into the destination collection. Reading a descriptor
copies its bounds under the registry lock: returning a borrowed pointer could
race with another task destroying a shorter-lived view of the same object.
Compiler and runtime must be rebuilt together after these internal ABI changes.

## Allocations and coextensions

An allocator supplied to an access parameter is owned by the called activation,
with its accessibility identity established on entry. Its allocation is finalized
on normal or exceptional return. Argument-evaluation failures retain cleanup in
the caller's temporary collection. Forwarding an access parameter transfers its
identity without transferring this ownership again.

An allocator used for an access discriminant creates a coextension. A pending
parent allocation owns its children's finalization chain and arena before
constraints or initializers run. Successful construction publishes the parent;
failure unwinds the pending owner. Parent deallocation finalizes its coextensions.
Returned composite results pass caller-owned finalization/arena storage and the
result master's identity. A coextension created for that return is distinguished
from an ordinary borrowed reference, which must pass the normal return check.
Collections and protected receivers created as coextensions inherit the owning
object's identity. Copying pointer values does not extend the owner's lifetime.

## Evidence and outstanding ACATS dependencies

`anonymousaccess*`, `anonymouscoextensions`, `anonymousprotected` and
`anonymousdispatch` cover scalar/array/component views, constant protection,
renaming, forwarded parameters, nested and protected callbacks, callback results,
controlling access, extended returns, coextensions, exception cleanup and four
concurrent workers. They run in whole/separate compilation and determinism modes.
`anonymousarrayobjects` additionally checks inline arrays, distinct allocations
for `others => new Cell`, grouped bounds/initializer evaluation, multidimensional
and inferred bounds, aliased elements, generics and library elaboration.
`runtime.access_levels` checks caller/callee ancestry, sibling task identities,
master address reuse and coextension ownership. Storage/type unit tests check
metadata layout, descriptor snapshots and allocation-failure cleanup. Runtime
ASan/UBSan and TSan instrument C runtime operations; QBE-generated Ada memory
accesses are not instrumented.

Unchanged C460001, C460002 and C3A2002 execute as PASS. C3A0013 now parses its
access discriminants; private extensions and limited current-instance Access
are implemented in [Phase 6f](tagged-discriminants.md). Its remaining compile
XFAIL is array type `Object'Range` syntax (P8). C3A2003 additionally needs range-attribute aggregate choices
(P8); formal access-type cases C3A2A01/C460A01/C460A02 remain 6g. No edits to
upstream test bodies weaken these requirements.

Controlling anonymous access results use the existing hidden accessibility
result word and dispatch ABI; see the
[overriding/access-result contract](overriding-null-procedures.md).
