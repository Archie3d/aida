# Task and protected construction

Task and protected objects, and supported records/arrays containing them, use
Ada 2005 limited build-in-place construction. Aggregates may use component
constructors or `<>`; component defaults may call constructors. Simple returns
of aggregates/constructor calls and extended returns initialize the final object
without copying task identities or protected locks. Unconstrained array results,
forwarded calls, access-to-function calls and allocators use the same paths.
Copying, assignment and predefined equality remain illegal.

Task-bearing function profiles carry the caller's master, collection and
activation group alongside the existing result storage/ownership parameters.
The callee's local tasks retain their own master. Result tasks stay dormant
through nested constructors and extended-return statements. Stand-alone objects
join the caller's declaration activation group; allocators activate after the
whole allocated object is initialized. Anonymous task-bearing argument results
and aggregates use temporary collections, activate after construction, and wait
before temporary protected/controlled components are finalized or storage is
released. This follows [RM 9.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-2.html).

Result construction takes a checkpoint in the owning master's existing task
list. Failure or an abandoned extended return discards only later tasks in the
same activation group before finalization/storage rewind. A call-site guard also
handles propagation from the constructor's local finalizers. Discard preserves
the pending exception and leaves other groups intact; a handler may retry.
Task-list access uses the existing shared master mutex. Protected results use
caller-owned finalization registrations and carry their destination accessibility
level through constructors and allocations.

Task/protected discriminants and bound callable views follow the
[concurrent-view contract](concurrent-callable-views.md). Tagged/variant composites
containing these objects and discriminant-dependent arrays of concurrent elements
remain diagnosed. Entries/rendezvous, [select](select-statements.md) and
[requeue/abort](requeue-abort.md) are implemented for their documented surfaces;
concurrent interfaces remain open. This increment
does not claim complete tasking or annex conformance. Recompile affected Ada
objects with the updated compiler/runtime; semantic interfaces already check the
compiler digest.

`concurrentresults` checks task/protected constructors, indirect and forwarded
calls, boxes/defaults, dynamic arrays, declaration activation, allocator and
argument temporaries, exception retries, abandoned returns and failing local
finalizers. It runs in whole-program and separate-unit modes, deterministic IR
checks, generic type/function instances and formal `in` objects, and four concurrent
callers.
`concurrentlibrarymain` checks exported limited-private constructors and indirect
forwarding across whole/separate-unit compilation. `semantics.returns` retains copy-rejection
controls; `runtime.tasking` checks explicit caller ownership, checkpoint/group
isolation, exception preservation and activation after retry. The sanitized
tasking campaign includes this fixture in whole/separate modes, with generated
memory-access instrumentation under TSan.

Local Darwin arm64 validation: the full 1,984-test suite passed, followed by 27
focused checks for the final generic-formal-object extension. ASan/UBSan passed
16 whole/separate tasking executions; TSan passed 18 including its intentional
race-detection controls. The concurrent-result fixture had 1,475 instrumented
memory-access sites in each TSan compilation mode. No broader host or language
conformance claim follows from these checks.
