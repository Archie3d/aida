# Aggregate box defaults

The baseline is Ada 2005 RM
[4.3.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-3-1.html)
and [4.3.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-3-3.html).

Record and extension aggregate associations can use `Component => <>`,
`Left | Right => <>` or `others => <>`. Each selected field evaluates its own
component declaration default, or receives ordinary default initialization of
its subtype. Grouped boxes may select different types. An empty others box is
legal after all needed fields have explicit associations; a record with no
needed fields still requires `null record`.

Array associations such as `(1 | 3 => <>, others => Value)` and
`(Value, others => <>)` initialize each selected element independently. A box
cannot replace a positional expression or a multidimensional subaggregate;
use `(others => (others => <>))` for a matrix. Array shape and choice checks run
before component initialization. Null ranges evaluate no component defaults.

Access components receive null explicitly. Scalars without a declaration
default have the ordinary unspecified initial value; the feature promises no
zero value for them. Runtime component subtypes retain their captured bounds,
and default expressions undergo the same checks as explicit initializers.

Construction uses the existing temporary, collection and finalization paths.
Completed controlled components are finalized if a later default fails;
limited elements initialize directly in their destination ([return construction](returns.md)).
Failed temporary construction leaves an assignment target unchanged.

Defaulted discriminants now use the [Phase 6c contract](default-discriminants.md);
boxed defaults selecting variants must be static. Runtime explicit constraints
use the [per-object component contract](runtime-discriminants.md). General
limited-record definitions/aggregates landed in 6e; task/protected aggregates
remain in Phase 9. Boxes do not circumvent these diagnostics. Runtime-sized tagged and
delayed private-component layouts follow the [Phase 6f contract](tagged-discriminants.md).

`boxaggregates`, `boxchecks`, `boxcleanup` and `boxtasking` cover grouped,
explicit and others associations, nested defaults, extensions, generics,
library runtime bounds, concurrent contexts, checked defaults and cleanup.
They run whole/separate and deterministic-IR checks. The runtime cleanup paths
were checked with ASan/UBSan and task contexts with TSan; generated QBE memory
accesses are not instrumented. Frontend UBSan checks cover compilation and
rejection cases. `aggregateboxerrors` and `boxpositionalerrors` cover legality.
Unchanged ACATS C433001 checks dynamic others constraints, not box defaults.
