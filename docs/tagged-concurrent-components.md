# Tagged records with concurrent components

Limited tagged records and record extensions may contain task and protected
components, including inherited components. Default initialization and extension
aggregates with ancestor subtype marks or limited ancestor aggregates construct
the components in their final storage. Ancestor initialization preserves the
extension's tag and inherited component offsets. Existing task activation groups,
masters and protected finalization registrations manage the whole object; no
new runtime state or ABI is needed.

Construction uses the [concurrent-result ownership rules](concurrent-results.md).
Task identities and protected locks are never copied. Extensions with a
nonlimited parent or progenitor reject limited components, as required by
[RM 3.9.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-9-1.html). Limited copies,
assignment, predefined equality and streaming remain diagnosed. Untagged
derivation of concurrent-containing types, variant concurrent records and
protected-containing deferred layouts retain their existing restrictions.
Discriminant-dependent arrays of concurrent elements remain unsupported.

Unchanged C432005 exercises both ancestor subtype marks and ancestor aggregates
across three tagged levels, with independent task entries and protected operations
in inherited and extension components. `runtime.acats_c432005_modes` runs it in
whole-program and separate-unit modes with the local REPORT implementation.
`taggedconcurrent` additionally checks default construction, a controlled ancestor,
exact finalization counts, ancestor views, protected exception recovery and
construction failure after inherited concurrent components have been initialized. Four
concurrent callers each repeat construction five times; each invocation creates
three objects with two tasks and two protected components. `taggedconcurrenterrors` rejects concurrent components with a nonlimited
ancestor, limited copies, assignment and equality. Execution, deterministic
emission and whole/separate-unit checks also run under ASan/UBSan.
