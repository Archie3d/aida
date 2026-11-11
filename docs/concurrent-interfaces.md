# Concurrent interfaces (Phase 9)

Task and protected types and single objects can implement limited interfaces.
Task declarations accept synchronized/task contracts; protected declarations
accept synchronized/protected contracts. Ordinary nonlimited contracts and
incompatible task/protected ancestry receive diagnostics. Transitive and diamond
ancestry share the existing interface maps. Repeated progenitors add one
ancestor and one inherited operation set.

An inherited primitive whose first parameter is the concurrent type, or an
anonymous access parameter designating it, can be implemented by a conforming
single entry or protected operation. The remaining profile must be subtype
conformant. Protected procedures and entries require a variable controlling
parameter. Overriding indicators are checked against these implementations;
explicit ordinary primitives cannot also implement the same operation through
an entry/protected operation. Inherited null procedures remain callable. Missing
abstract implementations are diagnosed. Entry families do not implement these
single-operation obligations.

Class-wide value/access dispatch, interface cross-conversions, membership and tag
queries retain the complete object's address. Checked conversions back to a
specific concurrent object produce views and preserve its synchronization and
ownership; failed conversions raise Constraint_Error. Concurrent objects remain
noncopyable. Access to inherited callable views uses an ordinary callback adapter,
with the existing accessibility and elaboration checks.

Synchronized private extensions can complete with task or protected types.
The full view must preserve the advertised interfaces and private visibility.
Synchronized formal derived types accept corresponding concurrent actuals,
including defaulted discriminants when the formal permits unknown discriminants.
Formal subtype and discriminant constraints continue to apply. Nested/library generic replay
uses the existing primitive-origin maps. Protected bodies see both their
protected declarations and their enclosing body's context and declarations.

## Dispatch and select

Calls through interface views use generated adapters to rendezvous with task
entries or acquire the protected execution resource. Direct protected calls
resolve to the protected operation without adding a second inherited candidate.
Internal calls retain the existing protected-action rules.

Conditional, timed and asynchronous selects through dispatching limited-interface
procedures preserve the caller's parameter objects and normal-return copy-back.
Entry adapters apply the existing queue/deadline/cancellation protocol. An ordinary
procedure implementation completes the triggering call directly; an asynchronous
select then skips its abortable part. Exceptions bypass copy-back.

The dispatch table still stores code/link pairs. Concurrent dispatch adapters
accept an optional trailing selection descriptor; ordinary implementations ignore
that trailing argument. The adapter's ordinary callback entry keeps its original
ABI. Selection descriptors are invocation-local, with no shared mutable dispatch
state. Asynchronous adapters borrow the caller's formal objects and copy their
operand packet into request-owned storage before returning, so no queued request
retains an adapter's stack frame. Requeue retains the Ada 2005 entry-name rules;
the later Synchronization aspect and procedure-requeue extension are outside the
target language.

## Object ABI and lifetime

Every task/protected object now has a 16-byte header: tag at offset 0 and runtime
resource at offset 8. The tag slot is unused for untagged objects. Discriminants
and private components follow the header. `common/Concurrent.h` defines the shared
C representation and compiler size, including captured runtime layouts; native
assertions and the host ABI check
validate the resource offset. The canonical address works for interface access,
barriers, protected callbacks, task current-instance access and runtime ownership.
The tag descriptor layout and interface-map format remain unchanged.

Protected finalization drains the execution resource before private payloads and
coextensions, including through class-wide access/deallocation. Task activation,
masters and allocation collections retain their existing lifetime rules. Rebuild
Ada objects and native runtime clients with this compiler/runtime: the concurrent
object header changed. Driver compiler/runtime fingerprints invalidate old caches.

Protected entry families, task/protected subunits, formal packages and the
existing restrictions on concurrent composites remain open under their existing
owners. Nonconfirming concurrent representation clauses and streaming remain
restricted. This milestone does not establish full tasking or annex conformance.

## Evidence

`concurrentinterfaces` covers repeated/diamond ancestry, captured private-array
layouts, value/access controlling
parameters, task/protected dispatch, current-instance access, callback ABI, checked
conversions, membership, null procedures and class-wide allocation/deallocation.
`interfaceselect` covers conditional failure without copy-back, expired deadlines,
queued protected/task asynchronous calls and immediate ordinary implementations.
`interfaceprivate` covers synchronized private task/protected completions.
`concurrentinterfacelibrarymain` exercises synchronized generic actuals across units
with four concurrent clients. All four run normally, deterministically and in
whole/separate compilation modes; all join the sanitizer tasking campaign.
`semantics.concurrent_interfaces` supplies thirteen invalid-source controls.
Existing native lifecycle tests now use the shared object header. The sanitizer
campaign also includes `runtimecomponenttasking`, which exposed the stale header
size in captured private-component layouts.

Unchanged ACATS C940017 passes the complete REPORT/TCTouch protocol, exercising
protected-interface procedures, functions and entries across library units.
It was selected before probing from the pinned revision; its 2019 source uses
Ada 2005 syntax. C950001/C954027 were assessed and excluded because they use
the Ada 2012 Synchronization aspect. The manifest retains their hashes/reasons.
The baseline now has 154 cases: 124 executable C passes, three legal-source
rejections, one runtime failure and 26 raw B observations. B394A01–B394A05 now
reach ancestry/private-completion semantics; their reviewed grade snapshots remain
FAIL under unchanged policies. The independent B total remains 17 PASS / nine FAIL.

On Darwin arm64, 324/324 selected ASan/UBSan checks and 19/19 selected TSan checks
pass. Each tasking campaign has 66 clean whole/separate executions; TSan also
requires both intentional race reports (68 executions). The relaxed-turn race
control reports the intended race in 32 consecutive runs per mode. These selections do not
imply the entire regular suite ran under sanitizers.

The contracts follow Ada 2005 RM [3.9.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-9-4.html),
[9.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-1.html),
[9.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-4.html),
[9.7.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-7-2.html),
[9.7.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-7-4.html) and
[12.5.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-12-5-1.html).

The complete regular Darwin arm64 suite passes 2,076/2,076 checks, including
290 checks under the ACATS label and 124 strict executable backfill passes.
The label also includes harness/selection checks and is not an official
conformance grade. Source/hash validation and `git diff --check` pass;
the external QBE source is unchanged.

B394A02–B394A05 now grade PASS after reviewing each declaration span and semantic
family: 108 required sites and six optional private completions. The independent
`interfaceancestry` control executes legal record, task/protected, private-completion
and generic ancestry in whole/per-unit modes with deterministic IR. The grading
repair changes no compiler behavior or raw observations; all 161 raw observations
and upstream hashes remain unchanged. The separate total is 25 PASS / one FAIL;
B394A01 awaits its source/foundation review. See the
[site review](acats-grading.md#interface-ancestry-review-phase-12).
