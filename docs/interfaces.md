# Interfaces (Phase 6f)

Ordinary and limited interfaces support multiple progenitors, transitive
ancestry, diamond inheritance, abstract operations and inherited null procedures.
A concrete ancestor's matching operation can implement an interface obligation;
an explicit override replaces the corresponding slot for every interface view.
Shared inherited profiles use one concrete slot. Private-only interface entries
retain dispatch maps without introducing visible names in descendants. Concrete
types still have to
implement every abstract obligation. Renamings, defaults, controlling access
parameters/results, factories, equality and multiple controlling operands use
the existing dispatching-profile rules.

The rules follow Ada 2005 [RM 3.9.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-9-4.html),
[3.9.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-9-2.html),
[4.6](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-6.html)
and [7.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-7-3.html).
Interfaces are implicitly abstract and have no components or discriminants.
Their user-defined primitives must be abstract subprograms or null procedures.
Nonlimited interface ancestry prevents limited descendants. A tagged private
completion must preserve the interfaces advertised by its partial view; an
untagged private view does not expose its full view's interface ancestry.

## Representation and conversions

Objects retain one canonical address, with their actual tag at its existing
location. A tag owns a unique transitive interface list and immutable maps from
each interface's operation numbers to concrete primary-table slots. Each slot
still carries code and its lexical link. Dispatch through an interface selects
that map, preserving the actual object's address, tag, layout and ownership.
The existing layout/dispatch descriptor prefix remains at the same offsets.
Local tags snapshot their activation's interface identities before registration;
metadata allocation failures publish no partial tag and reclaim earlier stages.

This avoids secondary object pointers and record-offset adjustments. Interface
views, access values, controlled/class-wide copies, allocation and deallocation
use the complete object's established storage and finalization mechanisms.
Class-wide interface cross-conversions and membership inspect the actual tag's
ancestry; failed checked conversions raise Constraint_Error. Null access
conversions preserve null. Unrelated specific conversions remain illegal;
specific view conversions retain ordinary static-dispatch rules.

Ada.Tags.Interface_Ancestor_Tags returns the unique interface ancestors of the
actual tag. Interface tags have no physical Parent_Tag. Existing tag names,
abstractness queries and same-level descendant queries include interface ancestry.

## Concurrent categories and remaining boundaries

Limited, synchronized, task and protected interface declarations are parsed and
classified. Their inheritance category rules are checked, including incompatible
task/protected combinations. Implementations by task/protected types and
synchronized private extensions remain Phase 9 work and receive diagnostics.
The slot maps contain no record-component offsets: those implementations can
provide wrapper entry points for protected actions and entries while preserving
the canonical object argument and interface identities.

Phase 6g adds interface formals of all five categories and derived contracts
with ordinary/limited interface ancestors and progenitors. Actual categories
match exactly; interface-origin maps preserve operations through reordered,
nested and library instances ([contract](generic-types.md#interface-formals)).
Synchronized formal derived extensions remain open. Concrete interface
implementations inside supported generic packages work, including separate
instances that share library-level interface contracts. General freezing,
storage pools, and the other previously documented type restrictions retain
their roadmap owners. This is a supported Ada 2005 subset, not a full conformance
claim.

## Regression evidence

`interfaces` covers multiple contracts, inherited concrete implementations,
class-wide and access cross-conversions, membership, nulls and deallocation.
`interfacediamonds` covers shared ancestors, null-over-abstract inheritance,
required overrides and tag queries. `interfaceownership` combines private views,
runtime-sized controlled records and repeated class-wide assignment.
`interfacecategories` checks limited implementations, nonlimited implementations
of limited contracts and the concurrent contract categories. `interfaceprofiles`
checks renamings, access results, contextual factory dispatch, equality and
multiple controlling operands. `interfacegenerics` checks two generic instances
against library contracts, including internal calls to private null procedures.
Each runs whole-program, per-unit and deterministic
compilation, with four workers and repeated local type elaborations.

`interfaceerrors`, `interfacecategoryerrors`, `interfaceprivacyerrors` and
`interfaceconversionerrors` pin illegal controls alongside legal declarations.
`runtime.interface_maps` checks map translation, copied metadata, lexical links
and invalid queries; `runtime.array_storage` injects failure at every local
interface-tag allocation stage. The 106-case ACATS backfill remains unchanged
at 60 executable passes, 18 compile rejections, eight harness blockers and 20
ungraded B observations. No additional upstream ACATS pass is claimed for these
local interface scenarios. Sanitizers instrument the compiler or C runtime;
QBE-generated memory accesses remain uninstrumented.

Validation on Darwin arm64: 1,115/1,115 CTest cases, 16 focused compiler
ASan/UBSan cases, and all six interface scenarios in whole/per-unit modes with
the instrumented C runtime pass. Instrumented interface-map, allocation-storage
and shared-table C tests pass as well. The complete ACATS cohort was reobserved;
only the recorded compiler fingerprint changed.
