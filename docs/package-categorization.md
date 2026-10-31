# Package categorization

Phase 7 enforces `Pure`, `Preelaborate` and
`Preelaborable_Initialization` for the compiler's supported Ada 2005 types.
The reference is Ada 2005 RM
[10.1.5](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-10-1-5.html),
[10.2.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-10-2-1.html)
and the external-streaming definition in
[13.13.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-13-2.html).

## Declarations and dependencies

A categorization pragma belongs at the start of a library package's visible
part, before nested declarations. Library subprogram declarations and generic
instantiations accept a following pragma naming that declaration. Positional
and `Library_Unit =>` notation are accepted. Nested, misplaced, duplicate and
incorrectly named categorization pragmas diagnose.

A pure unit may depend only on pure units; a preelaborated unit may depend on
pure or preelaborated units. Checks include parent declarations, body contexts,
private with clauses and expanded subunit contexts. The original Ada 2005 text
leaves the category of limited views undefined; AIDA conservatively checks the
originating package's category. The later rule treating every limited view as
pure is not claimed here.

Preelaborated declarations and bodies precede ordinary parts in both whole
program and separate compilation. Scans record each part's category in the
manifest. The binder checks the category order independently of explicit
elaboration edges; category scheduling does not turn limited with clauses into
elaboration dependencies. Incremental source and compiler digests cover these
changes through the existing unit-record checks.

## Elaboration

Package elaboration rejects calls to nonstatic functions, nonstatic object-name
primaries, default creation without preelaborable initialization, and executed
statements other than null. Enabled assertions require preelaborable expressions;
exception handlers follow the same statement restrictions. It checks constraint expressions, nested package
parts, generic expansions and package subunits. Subprogram bodies establish
callable code: their local initialization and statements execute when called
and remain unrestricted. Attribute prefixes and object renamings are evaluated
as names; their index expressions and nested calls still need to be preelaborable.

Component defaults are checked when used. An explicit aggregate may override a
non-preelaborable default. Aggregate boxes, default-initialized ancestors and
allocators check the initialization they actually perform. Pure elaboration
also rejects variables and access-to-variable allocation, including the presumed
hidden allocation when default-initializing an opaque private component.

Ada 2005 permits named access-to-subprogram types and named access-to-variable
types with a statically specified `Storage_Size` of zero in pure units.
Anonymous access-to-variable types at library level in a pure unit have zero
storage; access parameters and access discriminants retain their normal pools.
Only static zero access-to-object Storage_Size clauses are implemented; other
storage-pool specifications diagnose. Direct zero-pool allocators are illegal.
An allocator in a generic body instantiated with a zero-pool actual raises
`Storage_Error`. Pure private full views with available stream attributes must
support external streaming; access parts require visible explicit marshalling.

## Initialization promises and generics

`Preelaborable_Initialization` accepts private types/private extensions and
entry-free protected types in the same visible package part, and formal private
or derived types in a generic formal part. It rejects inappropriate targets,
placement and duplicates. A private completion or protected component that
breaks the promise diagnoses; actual types must satisfy a formal type's promise.
Private view legality is captured at the point of object creation, so a later
completion cannot make an earlier opaque declaration legal.

Elementary types have preelaborable initialization; arrays, records, derived
types and interfaces follow the Ada 2005 component/parent rules. Tasks and
protected types with entries do not have it. A user-defined controlled type
with an overriding Initialize lacks it, even if that procedure is null; the
later null-Initialize relaxation is not used.

A library generic's category is not inherited by its instances. A categorized
instance is checked with its actual types, values and subprograms, including its
private part, formal-object bindings and bodies expanded later. A generic
specification establishes a template without executing its future declarations;
its preelaborated body is checked assuming nonstatic formal types and objects,
non-preelaborable private formal types unless promised otherwise, and user-defined
formal subprograms. This follows the original Ada 2005 rules, without importing
the later prohibition on templates for which no categorized instance is possible.

## Predefined environment and validation

The Ada root, character packages, Strings/Maps/Constants, Numerics and elementary
functions, Streams, IO_Exceptions, Containers and sorting utilities carry their
Ada 2005 Pure categories. Strings.Fixed/Bounded/Unbounded, container families
and shared storage machinery, Finalization, Exceptions, Tags, Command_Line,
Environment_Variables and Task_Identification carry Preelaborate. Applicable
private initialization promises are present in the predefined specs. Runtime
packages pass the same legality checks as user packages. Finalization remains
Preelaborate as in Ada 2005; its Annex E Remote_Types contract is outside this
compiler's distributed-systems support.

`categorization` executes whole-program and per-unit builds and checks deterministic
IR. `semantics.categorization` covers dependency categories, limited/private
contexts, child units, constraints, subunits, template versus instance rules,
streaming privacy, controlled initialization and tampered binder order.
The `categorization*` and `preelaborable*` rejection cases pin diagnostics.
Unchanged ACATS CA21001 exercises ordinary package subunits inside a categorized
library subprogram. Existing string/container ACATS cases continue to exercise
the newly categorized predefined implementations. No B-test grading or Annex E
conformance is claimed. Annex C.4's additional no-runtime-elaboration-code
requirements and the broader Phase 8 freezing/representation audit remain separate.

The [wide-library audit](wide-library-audit.md) adds Pure root/fixed string
hashes and Preelaborate bounded/unbounded hashes. The three A.4.9/A.4.7/A.4.8
mandated fixed hash renamings retain their specified Pure category despite
their Preelaborate parents; the loader's exception is confined to those exact
renamings and implicit parent edges. Other dependencies retain the normal rules.
