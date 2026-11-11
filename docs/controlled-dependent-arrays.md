# Controlled components in discriminant-dependent arrays

A record may contain an array of controlled elements whose bounds depend on
its discriminants. Aggregate initialization constructs the controlled target
directly without calling its Adjust; extension aggregates retain their ancestor
initialization rules. Default initialization calls Initialize, while copying and
assignment call Adjust on the copied elements. This follows Ada 2005 RM
[7.6](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-7-6.html)
and [7.6.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-7-6-1.html).

Each managed array component has a descriptor, allocation arena and its own
finalization chain. Allocation reserves inactive registrations before element
construction, and completed elements activate their registrations. Empty arrays
construct no elements. Finalization processes the element chain before releasing
its arena, including cleanup after a partially failed initialization.

Copying clones the payload and registers the copy's elements independently.
Copied tags and layout pointers remain attached to the actual objects, so helper
functions do not need a vanished declaration frame. Adjust operates on the copy;
failed adjustment cleans up completed parts without touching the source. An
ancestor-view copy retains the existing separation between component owners.

`controlleddependentarrays` checks aggregate initialization without Adjust,
ordinary default construction, empty arrays, independent copying, assignment,
failed Adjust and partially failed initialization with live-object counters.
It runs whole-program, per-unit and deterministic IR checks, with four workers
repeating the same ownership/failure checks using independent local state. Generated QBE
memory accesses remain outside compiler/runtime ASan instrumentation.

Task/protected elements in discriminant-dependent arrays remain diagnosed.
Explicit record representation clauses for managed dependent arrays also remain
unsupported: their inline representation cannot hold the ownership descriptor.
`discriminantcomponenterrors` and `controlleddependentarrayerrors` retain these
rejections.

## C761010 integration

The unchanged C761010 test additionally needs access-to-array constraints and
[deferred indefinite constants](access-subtype-constraints.md#array-constraints-and-deferred-constants-phase-12).
The binder prefers an available parent body before a child body when this does
not conflict with dependency or Elaborate_Body ordering. This avoids unnecessary
access-before-elaboration failures in parent operations used by the child.
`childbodyelaboration` checks both the preference and a parent-body dependency
on an immediate child spec/body pair, which must take precedence.

C761010 now completes its original REPORT protocol, including library-object
finalization, in whole-program and per-unit builds. CC3605A also passes unchanged
after the shared access-constraint repair. Upstream sources and hashes are
preserved; neither REPORT assertions nor grading rules are relaxed.

## Validation

Darwin arm64 passes all 2,170 Release checks, including 308 ACATS checks, and
42 expanded ASan/UBSan checks. The final concurrent fixture passes three further
checks in each build (execution, deterministic IR and whole/per-unit modes).
The full 161-case baseline recheck passes: only C761010 and CC3605A are promoted,
with 132 executable passes and three retained C rejections. All 26 B grades and
165 upstream source hashes are unchanged. The Hello binder golden changes only
the reviewed parent Text_IO / child Scanning body order. `git diff --check`
passes, and the external QBE source is unchanged.
