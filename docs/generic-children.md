# Generic children of generic parents

Following Ada 2005 [RM 10.1.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-10-1-1.html),
library generic packages expose their generic children through their checked
specification. A child template is implicitly bound to each parent instance
when selected. That view has its own contract and formal bindings, while its
parent declarations denote the types, objects, exceptions and operations of
that particular parent instance. A second child instance can share its parent's
state while retaining separate declarations and state of its own.

Children of children use the same mechanism. Generic child packages and
subprograms retain parent private visibility in their bodies. Public child
specifications retain the ordinary privacy restrictions. A client's access to
a generic parent does not expose its ordinary template declarations or make
unmentioned child library units visible.

The bound contract replays names resolved in the original template and maps
parent declarations to their instance copies. Context layers from both the
child specification and a separate body retain imported names and defaults;
only the enclosing parent environment changes. Formal-package matching uses
the bound child template identity, so a child from a different parent instance
cannot satisfy the formal, even when both parents have identical actual types.
Binding follows the selected parent instance, independently of where the
child instance is declared. Direct instantiation of the original generic child
outside its parent's region is diagnosed.

Source discovery retains the child specification/body and parent dependencies
in specification-only and whole-program loading. Nongeneric children of a
generic package and ordinary child declarations beneath a generic package
instance are diagnosed; library-unit renamings retain the RM 10.1.1(16)
exception. The generic body elaboration flag is owned by the
original, fully qualified library template; bound views reuse that flag.
Instances retain the normal elaboration checks and emit their own expansions.
Formal-subprogram wrappers use the instance name and declaration location, so
independent compilation does not depend on which library instance was seen first.

A bundled source can place the child body after its instances. Bound contracts
then attach the completed source body and resume the pending expansions with
their original specification symbols and parent bindings. This preserves type
and callable identity instead of constructing another specification.

`genericchildren` covers split-source and library instances, inherited private
state, distinct parent and sibling-child state, imported formal defaults,
parent exception identity, private-child helpers, child subprograms, library
renamings, independently placed child instances and nested generic children. The
main task and four workers exercise independent local instances.
`genericchildlate` covers late body attachment in a source bundle. Both have
whole/per-unit deterministic IR checks; the four-worker scenario also runs the
whole/separate execution harness. The `genericchild*errors`,
`genericchildvisibility` and `genericchildnamespace` fixtures check parent kinds,
wrong parent/template/type identity, privacy, context visibility and namespace
collisions.

The complete unchanged CC70001 executes as PASS, including its generic child
of a library parent instance and formal-package list-processing scenario.
Source provenance is recorded in the [ACATS inventory](../tests/acats/README.md).
Grouped generic formal objects and Pack retain their separately queued Phase 8
items; this increment adds no follow-on roadmap items.

Validation: the complete Release suite passed 1,755/1,755 checks. After the final
name-encoding adjustment, all 16 focused naming/visibility and container controls
passed again in Release and ASan/UBSan. The complete 129-case backfill refresh
retains all outcomes (92 executable passes, ten compile rejections, one support
blocker and 26 ungraded B observations); CC70001 is a separate focused pass.
