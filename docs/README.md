# AIDA documentation

Start with the [build and hello-world example](../README.md). The
[Ada 2005 coverage matrix](ada2005-coverage.md) lists supported behavior,
implementation choices and known gaps by Ada Reference Manual clause. The
[roadmap](../TODO.md) tracks planned work, and the
[roadmap history](roadmap-history.md) summarizes completed milestones.

## Building Ada programs

- [Driver builds and cleanup](driver-builds.md) covers incremental builds, output
  modes, linking with C objects and archives, and artifact cleanup.
- [Unit source layouts](unit-source-layouts.md) covers library roots, unit
  discovery, source bundles and separate bodies.
- [Compiled semantic interfaces](semantic-interfaces.md) covers packaging checked
  library units for use without their Ada sources.
- [Compiler diagnostics](diagnostics.md) explains format, source spans and
  related notes.
- [Host and target validation](host-validation.md) lists supported toolchains,
  ABI checks and installation tests.
- [C interoperability](interfaces-c-abi.md) covers `Interfaces.C` and its child
  packages, supported calls, target layouts and limitations.

## Language features

| Area | Contracts |
| --- | --- |
| Numeric types and checks | [Static arithmetic](static-arithmetic.md), [checked arithmetic](checked-arithmetic.md), [floating point](floating-point.md), [modular types](modular-types.md), [ordinary fixed point](fixed-point.md), [decimal fixed point](decimal-fixed-point.md), [runtime scalar constraints](runtime-scalars.md) |
| Arrays, records and expressions | [Array descriptors](array-descriptors.md), [discrete ranges](discrete-ranges.md), [aggregate defaults](aggregate-defaults.md), [case selectors](case-selectors.md), [runtime array storage](runtime-composites.md), [default discriminants](default-discriminants.md), [runtime discriminants](runtime-discriminants.md), [composite equality](composite-equality.md) |
| Representation | [Representation clauses](representation-clauses.md), [packed arrays and unchecked conversion](packed-arrays.md) |
| Access and results | [General access](general-access.md), [anonymous access](anonymous-access.md), [designated subtype constraints](access-subtype-constraints.md), [null exclusions](null-exclusions.md), [unchecked access](unchecked-access.md), [subprogram access](subprogram-access.md), [foreign results](foreign-results.md), [extended returns and limited construction](returns.md) |
| Object orientation | [Abstract subprograms](abstract-subprograms.md), [overriding and null procedures](overriding-null-procedures.md), [operator visibility](operator-visibility.md), [tagged discriminants](tagged-discriminants.md), [interfaces](interfaces.md), [prefixed calls](prefixed-calls.md), [bound prefixed views](prefixed-views.md) |
| Program structure | [Enclosing names](enclosing-names.md), [limited and private context views](context-views.md), [subunits](subunits.md), [package categorization](package-categorization.md), [pragmas and checks](pragmas-and-checks.md) |
| Generics | [Generic formals and contract checking](generic-types.md), [generic child units](generic-children.md), [grouped formal objects](generic-grouped.md) |
| Controlled types and lifetimes | [Assignment and adjustment failures](controlled-assignment.md), [controlled variants](controlled-variants.md), [controlled dependent arrays](controlled-dependent-arrays.md), [shared access-type collections](shared-collections.md) |

## Tasking

- [Task entries and rendezvous](task-entries.md),
  [select statements](select-statements.md), and [requeue and abort](requeue-abort.md)
  cover queues, cancellation, transfer and cleanup.
- [Concurrent construction](concurrent-results.md),
  [concurrent callable views](concurrent-callable-views.md),
  [concurrent interfaces](concurrent-interfaces.md), and
  [tagged concurrent components](tagged-concurrent-components.md) cover object
  identity, result ownership, discriminants and dispatch.
- [Task services](task-services.md), [task termination](task-termination.md),
  and [ceiling locking](ceiling-locking.md) cover hosted runtime behavior.
  The [selected annex follow-up goals](selected-annex-followups.md) describe
  additional facilities and profiles.

## Predefined library

Use the [predefined environment inventory](predefined-environment.md) to check
which units ship with the compiler. The contracts below describe their behavior.

| Area | Contracts |
| --- | --- |
| Standard, System and storage | [Standard and System](standard-system.md), [storage elements](storage-elements.md), [address conversions](address-conversions.md), [storage I/O](storage-io.md), [storage pools](storage-pools.md) |
| Strings and containers | [Unicode source and wide characters](wide-foundation.md), [wide strings](wide-strings.md), [wide library inventory and hashing](wide-library-audit.md), [unbounded strings and containers](library-collections.md) |
| Text and stream I/O | [Text I/O](text-io.md), [wide text I/O](wide-text-io.md), [streaming](streaming.md), [stream files](stream-io.md), [text stream views](text-streams.md) |
| Calendar and host services | [Calendar and its child units](calendar.md), [directories and environment variables](host-library.md) |
| Other library services | [Exceptions](exceptions-library.md), [random generators](random.md), [dispatching constructors](dispatching-constructors.md) |

## Compiler internals and validation

The `ada/` driver manages dependencies, binding, caching and toolchain
invocation. The `adac/` frontend analyzes Ada and emits QBE IR. QBE produces
assembly for the system toolchain, which links the program with the C runtime
and compiled predefined units from `runtime/`. Shared code lives in `common/`
and `support/`.

- [Direct QBE lowering](lowering-decision.md) records the code generation
  decision and its implications for storage, cleanup and concurrency.
  [Abort deferral](abort-deferral.md) explains the runtime regions and emitter effects.
- [Artifact publication](artifact-publication.md) describes build directory locking,
  atomic publishing and dependency consistency.
- [Robustness validation](robustness.md) covers deadlines, deterministic fuzzing and
  sanitizer configurations.
- The [ACATS test guide](../tests/acats/README.md) explains how to run the
  suite. The [baseline](acats-baseline.md) records outcomes by case and the
  [B-test grading contract](acats-grading.md) explains independent legality review.
  Historical selection decisions are in the [adoption audit](acats-adoption.md),
  [legality audit](legality-audit.md), and [supported-area rejection audit](supported-rejections.md).
- [Performance validation](performance-wrap-up.md) explains how to compare builds
  and links each measured milestone to its raw report. Detailed studies cover
  [runtime overhead](runtime-profiling.md), [block-master elision](lexical-master-elision.md),
  [aggregate helpers](helper-profiling.md), [scratch zeroing](inline-scratch-zeroing.md),
  [offset and copy costs](residual-helper-profiling.md), and
  [temporary finalization](finalization-scaling.md). The [evidence directory](evidence/)
  holds the original JSON reports. Paths inside them describe the recorded runs.
- [Ada fixtures](../tests/ada/README.md) are runnable language examples, and the
  [benchmark fixtures](../tests/benchmarks/README.md) document ABI and performance probes.
