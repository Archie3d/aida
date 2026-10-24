# Named general object access

Phase 6d supports `aliased` variables/constants, aliased record components and
array elements, named `access all` and `access constant` types, and contextual
object `'Access`. [Object `Unchecked_Access`](unchecked-access.md) subsequently
added the same object-view rules with library-level accessibility. Renaming preserves aliasing and accessibility. Tagged formal
parameters are implicitly aliased. Dereferences of access-to-constant values
are constant views; an access-to-variable component stored in such a view still
permits modification of its designated object.

The legality checks follow [RM 3.10.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-10-2.html)
and [RM 4.6](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-6.html):
non-aliased objects, constant-to-variable access, incompatible designated
subtypes and statically deeper objects/types are rejected. Derived access types
retain the ancestor's accessibility level. Generic package bodies retain
run-time accessibility checks when replayed at a deeper instantiation level;
failure raises Program_Error. Named access conversions preserve object identity,
check tags and designated record constraints, handle null without dereferencing,
and cannot remove constant protection. Statically sized designated storage representations and alignments must match.
Taking Access is also an address escape for the existing shared-storage checks.

Arrays require statically matching nominal designated subtypes. For example,
`X : aliased String := "abc"` has an unconstrained nominal subtype and can be
accessed by `access all String`; an explicitly constrained object requires a
matching constrained designated subtype. Pointer values remain raw object
addresses. Unconstrained array bounds use immutable descriptors registered in
the existing shared collection index. Collections own borrowed metadata only,
never the aliased object's storage or controlled lifetime. Metadata is keyed by
address, nominal type identity and array rank, with separate ownership for nested
collections. Owned array allocation metadata also records its type identity.
Compiler and runtime must be rebuilt together after this internal ABI change.

Ada 2005 allows default-discriminated aliased variables to remain unconstrained
([RM 3.3.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-3-1.html)).
General mutable record access preserves actual constrainedness through metadata;
assignment and in-out argument passing use it, and evaluate the pointer prefix
once. Constant views remain constrained. Access to a discriminant-dependent
component of an unconstrained variable is rejected because its storage can
change on assignment. The analysis conservatively rejects such component Access
through an unconstrained general mutable access view or unconstrained formal,
even if a particular actual object is constrained.

[Anonymous access and access discriminants](anonymous-access.md) landed in the
following Phase 6d item. [Null exclusions](null-exclusions.md) are supported;
broader generic formal access types remain separate roadmap items. Current limited instances
need the broader limited-type work in Phase 6f. Constraints on access subtypes,
custom storage pools, and general representation freezing remain Phase 8 work;
unsupported syntax receives diagnostics. Applications must synchronize concurrent
modification of designated data, as with ordinary shared object access.

Evidence: `generalaccess`, `generalaccessarrays`, `generalaccesschecks`,
`generalaccesstasking` and `generalaccesserrors`; unit tests for subtype flag
inheritance, borrowed descriptor/constrainedness lifetime, overlapping addresses,
allocation failure cleanup and accessibility checks. Executable regressions run
in whole/separate modes and determinism checks. The four-worker case shares
borrowed array views across collections. Runtime ASan/UBSan and TSan checks
instrument C runtime operations; generated Ada loads/stores are not instrumented.

Unchanged ACATS C3A0013 is a pinned compile XFAIL for private extensions and
package renaming (anonymous access discriminants now parse); it also requires
limited current-instance Access (P6f) and a range attribute as an array type
index range (P8). It is not an executable pass. C3A0014 in the selected GCC
mirror tests the superseded Ada 95 rule that
aliased default-discriminated objects are constrained, so it is inapplicable to
this Ada 2005 behavior. C3A0015 requires custom storage pools.
C3A2002/C460001/C460002 now pass with
anonymous parameters; C3A2003 still needs range-attribute aggregate choices.
C3A2A01/C460A01/C460A02 require broader generic formal access types (6g).
