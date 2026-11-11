# Concurrent discriminants and callable views

Task and protected types accept discrete and anonymous access discriminants,
constraints and defaults. Discriminants are immutable and visible to the current
instance, including nested subprograms; protected discriminants are selectable
outside the object while private components remain hidden. Defaults evaluate
per object before task activation. Existing null, constraint, accessibility and
coextension rules apply. Single task/protected objects cannot declare discriminants.

The [concurrent-interface ABI](concurrent-interfaces.md) now reserves a 16-byte
header: tag at offset zero, task/protected resource at offset eight. Discriminants
follow the header. Protected private arrays and plain managed records
use object-owned component views for discriminant-dependent bounds. Each task
entry-family bound is evaluated against that task's immutable discriminants,
including calls through a bound view. A discriminant must appear alone as a
family bound; more complex dependent expressions are diagnosed.

Task entries, family members, protected entries and protected subprograms can
be renamed or supplied as generic formal subprogram actuals. A view captures its
receiver and family index once, at declaration/instantiation. Changing an access
variable or index variable afterwards does not redirect it. Null access receivers
raise Constraint_Error while binding. Limited function-result receivers retain
their existing master-owned lifetime. Profiles, original formal subtypes, new
formal names and defaults follow the normal renaming machinery.

These are procedure/function views, not new entry declarations. Select lowering
follows a statically bound entry through renaming chains; entry attributes and
requeue still require entry declarations. Internal protected operation views
reuse the held execution resource. Generic named concurrent defaults resolve in
the generic declaration's scope and capture a fresh receiver per instantiation.
Entries cannot designate access-to-subprogram values, including through renaming
or a generic actual.
Access to a generic formal subprogram itself is rejected during contract
checking, before an entry or protected actual is bound; the
[Phase 12 formal-profile rule](generic-types.md#access-to-generic-formal-subprograms)
applies independently of the actual's kind.

Named access-to-protected-procedure/function types use the existing immutable
code/environment/receiver descriptor. Calls retain protected synchronization;
null calls raise Constraint_Error. Matching protected access conversions retain
the descriptor and enforce profile/accessibility restrictions. Equality compares
code, environment and receiver, including inside arrays and records; two freshly
created descriptors for the same operation/object compare equal. Anonymous
callback parameter renamings preserve the parameter's type and lifetime.

Protected destruction drains the execution resource before releasing private
component storage or default-created access-discriminant coextensions. Partial
construction registers cleanup before evaluating component defaults. Default
coextensions of library objects belong to the library construction owner rather
than the temporary elaboration chain. A finalization-record promotion preserves
this order when coextension records are registered after the protected parent.

Concurrent interfaces follow their [contract](concurrent-interfaces.md).
Protected entry families, tagged or
variant composites containing concurrent objects, and controlled/task/protected private component parts remain
outside this increment and are diagnosed. The existing foreign callback ABI
limits remain. This is not a claim of complete tasking or annex conformance.

## Evidence

`concurrentdiscriminants`, `concurrentviews`, `concurrentviewdefaults` and
`concurrentviewlibrarymain` run normally, deterministically and in whole/separate
compilation modes. They cover per-object bounds/defaults, access receivers,
family-index capture, select, generic defaults, protected internal calls,
callback identity/conversion, library elaboration and exceptional cleanup.
`semantics.concurrent_views` checks eighteen invalid declarations/uses.
`runtime.protected_lifecycle` adds a concurrent destruction/coextension-order
control. The sanitizer tasking campaign includes all four fixtures.

Eight sources selected from the pinned ACATS revision execute unchanged with
the complete REPORT success protocol: C910001, C940012, C940015, C85014B,
C85018A, C85018B, C3A0018 and C3A0027. Source hashes and language reviews are in
the [manifest](../tests/acats/baseline/manifest.json). The expanded baseline has
123 executable passes, three legal-source rejections, one runtime failure and
26 ungraded B observations. B330001/B3A0002 now reach semantic diagnostics;
rejection alone does not establish a B-test pass.

The language contracts follow Ada 2005 RM [3.7](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-7.html),
[8.5.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-8-5-4.html),
[9.5](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-5.html),
[9.7.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-7-2.html),
[12.6](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-12-6.html) and
[3.10.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-10-2.html).

On Darwin arm64, the selected ASan/UBSan checks pass 324/324 and TSan checks
pass 19/19. The tasking campaigns execute 56 clean whole/separate runs each;
TSan additionally verifies both intentional race reports (58 executions total).
These selected sanitizer checks do not imply the entire regular suite was
run under sanitizers.

The complete regular Darwin arm64 suite passes 2,062/2,062 tests, including
289 checks under the ACATS label (123 executable backfill passes). The label
also includes harness/selection checks; it is not an official conformance grade.
`git diff --check` passes and the external QBE source is unchanged.
