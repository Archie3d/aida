# Roadmap history

The implementation progressed from diagnostics and runtime foundations through
the Ada 2005 subset, validation tooling and native C interoperability. See the
[current roadmap](../TODO.md) for open work and the
[coverage matrix](ada2005-coverage.md) for current limits. Detailed checklists
and intermediate results remain available in Git history.

## Phase 1 — Stop silent acceptance

Diagnosed unsupported syntax, pragmas and representation items whose semantics
had been dropped. Later phases implemented subsets of these constructs with
execution and legality tests. [Pragmas and checks](pragmas-and-checks.md).

## Phase 2 — Tasking foundations

Moved exception state and trace frames into per-task context, added runtime
threading and masters, synchronized shared registries, and introduced
caller-owned result storage. [Task-context ABI](../tests/benchmarks/README.md).

## Phase 3 — Harden the existing subset

Established unit identity and binder elaboration checks. Strengthened
visibility, conformance, aggregate checks, array sliding, derived profiles,
primitive inheritance and variant record layouts.
[Coverage by language clause](ada2005-coverage.md#language-clauses-and-cross-feature-behavior).

## Phase 4 — Minimal tasking

Implemented concurrent task activation, protected operations and entries,
`Duration`, delays, monotonic time, task identity, and scalar atomic and
volatile access. [Task and time coverage](ada2005-coverage.md#language-clauses-and-cross-feature-behavior).

## Phase 5 — Numeric and representation foundations

Added exact bounded static arithmetic, checked numeric operations, modular and
fixed-point types, and shared array descriptors. Established the coverage matrix
and selected ACATS harness. Retained direct QBE emission.
[Numeric coverage](ada2005-coverage.md#numeric-array-and-expression-coverage),
[array descriptor ABI](array-descriptors.md), [lowering decision](lowering-decision.md).

## Phase 6 — Complete the core language

- **6a. Tasking prerequisites.** Shared allocation collections, checked
  controlled assignment and abort deferral for runtime cleanup.
  [Shared collections](shared-collections.md), [abort deferral](abort-deferral.md).
- **6b–6c. Dynamic data and discriminants.** Runtime scalar constraints, array
  bounds, varying record layouts, controlled variants and per-object components.
  [Runtime scalars](runtime-scalars.md), [runtime composites](runtime-composites.md),
  [runtime discriminants](runtime-discriminants.md).
- **6d. Access types.** General and anonymous access, accessibility, null
  exclusions, coextensions and callbacks.
  [General access](general-access.md), [anonymous access](anonymous-access.md),
  [subprogram access](subprogram-access.md).
- **6e. Program structure and names.** Subunits, limited and private context
  views, limited full views, extended returns and build-in-place results.
  [Subunits](subunits.md), [context views](context-views.md), [returns](returns.md).
- **6f. Object orientation.** Refined tagged/private views, abstract operations,
  interface inheritance, prefixed calls and dispatch. Reviewed selected ACATS
  rejections in supported features.
  [Tagged views](tagged-discriminants.md), [interfaces](interfaces.md),
  [ACATS rejection audit](supported-rejections.md).
### 6g. Generics

Extended formal types, subprograms, objects and packages. Checked contracts for
derived, tagged, interface and access formals, nonstatic array constraints,
partial formal-package associations and limited `in` actuals.
[Generic semantics and scope](generic-types.md).

### 6h. Pragmas and checks

Implemented assertion policies, supported check suppression and `No_Return`
semantics. [Pragmas and checks](pragmas-and-checks.md).

## Phase 7 — Predefined library

Extended strings and containers, calendar and host services, exception
occurrences and streaming, UTF-8 source, wide strings and text I/O. Added
categorization checks for supported library packages.
[Predefined-unit coverage](ada2005-coverage.md#predefined-unit-inventory),
[wide-library audit](wide-library-audit.md).

## Phase 8 — Tooling and validation

Established native host and ABI checks, relocation controls, sanitizer
configurations, deterministic malformed-source tests and independent ACATS
legality grading. Extended representation and freezing checks. Added
hierarchical source lookup, source-free semantic interfaces, coordinated
artifact publication, no-op builds and source-free cleanup. Extended prefixed
renamings and generic actuals to the supported temporary receiver layouts.
[Host validation](host-validation.md), [robustness](robustness.md),
[ACATS grading](acats-grading.md), [compiled interfaces](semantic-interfaces.md),
[artifact publication](artifact-publication.md), [driver builds](driver-builds.md),
[bound prefixed views](prefixed-views.md).

### Predefined library implementation

The environment audit established import probes and executable controls for
partial APIs. Standard/System declarations and storage elements preceded
address conversions, storage I/O and custom storage pools. The increment also
added random generators, tag-selected construction, the remaining stream file
operations and the three text stream widths.
[Predefined environment](predefined-environment.md),
[unit inventory](../tests/predefined/inventory.json).

## Phase 9 — Core tasking

Added task entries and rendezvous, selective accept, timed and conditional
calls, asynchronous select, requeue, abort, concurrent callable views
and interface dispatch. Task and protected results reuse in-place construction.
Selected task services cover priorities, attributes, suspension objects and
termination callbacks.
[Task entries](task-entries.md), [select](select-statements.md),
[requeue and abort](requeue-abort.md), [concurrent construction](concurrent-results.md),
[concurrent interfaces](concurrent-interfaces.md), [task services](task-services.md),
[termination](task-termination.md).

## Phases 10–11 — Diagnostics and legality review

Added rich diagnostic spans, related declarations and generic-instantiation
notes, improved parser recovery and reconciled selected ACATS legality and
execution outcomes. [Diagnostics](diagnostics.md), [ACATS baseline](acats-baseline.md),
[B-test grading](acats-grading.md).

## Phase 12 — ACATS correctness follow-ups

Completed controlled dependent-array initialization (C761010 and CC3605A),
tagged concurrent components (C432005), packed signed arrays (CXC7002) and hosted
ceiling locking (CXDA003). The backfill records 135 executable passes and 26
raw B rejections. The independent grader records 26 B passes.
[ACATS baseline](acats-baseline.md#baseline-table-refresh-phase-12),
[controlled arrays](controlled-dependent-arrays.md),
[tagged concurrent components](tagged-concurrent-components.md),
[packed arrays](packed-arrays.md), [ceiling locking](ceiling-locking.md).

## Post-Phase 12 fixes

- Indexed heap finalization-chain ownership and avoided shared lookup after a
  successful task-local removal. [Finalization scaling](finalization-scaling.md).
- Restricted universal numeric overload candidates to visibly numeric types.
  [Operator visibility](operator-visibility.md).
- Fixed line reads on open pipes and terminals, and assigned Ada `Main` a
  distinct linker symbol from the binder's C entry point. [Text I/O](text-io.md).

## Phase 13 — Interfaces.C support

Added target-specific scalar representations, native C scalar and pointer call
profiles, character arrays and conversions, and the `Strings` and `Pointers`
packages. Validated the subset on all four supported hosts.
[C ABI contract and integration audit](interfaces-c-abi.md).
