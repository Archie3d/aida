# Remaining supported-area rejection audit

This completes the 6f-prelude review of the pinned 106-case
[ACATS baseline](acats-baseline.md). It is a review of this selection, not a
complete language audit. Upstream source bytes are unchanged. The exact
observations and per-case triage remain in
[expected.json](../tests/acats/baseline/expected.json) and
[manifest.json](../tests/acats/baseline/manifest.json).

## Completed repairs

- **C41103A/C41105A:** an indexed component may have a parameterless function
  returning an array or access-to-array as its prefix
  ([RM 4.1.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-1-1.html)).
  Discovery considers both ordinary calls and indexing, using the component's
  expected type and each index type. Two viable interpretations remain
  ambiguous. Analysis binds the selected prefix once, preserving defaults,
  access null checks, writable designated components and constant array results.
  Named associations belong to calls and are rejected as indices. Slice
  indexing retains the slice's nominal bounds when lowering its data address;
  stripping the subtype previously paired that address with the parent's bounds.
- **C73002A:** a nested package body's specification supplies its immediate
  declarations; enclosing declarations come from the body's textual position
  ([RM 8.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-8-2.html)).
  A body-local scope view keeps the original declaration identities, privacy,
  use clauses and operator visibility while following the surrounding body.
  The specification's lexical chain stays intact, so clients cannot acquire
  body locals. Existing separate-subunit context handling is preserved.
- **C730004:** an untagged limited private type can have a nonlimited full view
  ([RM 7.3(6)](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-7-3.html)).
  Concatenation and predefined comparisons use the visible view's limitedness,
  including component views. Limited partial views outside the declaring
  package and genuinely limited full views/components remain restricted.
- **C37006A, partial repair:** discriminants declared in a private partial view
  are selectable by clients
  ([RM 4.1.3(7)](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-1-3.html)).
  The type retains the partial view's discriminant
  count through completion; discovery and binding expose those discriminants
  while keeping full-view components hidden. The case still rejects its
  independently unsupported access-subtype constraint at line 155.
- **C760011, partial repair:** `use type` accepts subtype marks with `'Class`
  or `'Base`, including qualified marks and comma-separated lists. It uses the
  existing type resolution rules; a class-wide mark contributes the specific
  type's primitive operators, including user-defined operators
  ([RM 8.4(8)](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-8-4.html)).
  The case still needs
  new discriminants on a derived tagged type, plus REPORT.Ident_Char support.

All four newly executable cases pass unchanged and join the strict selection.
`supportedrejections` and its library/private-child model check array/access
results, overloaded indices, defaulted parameterless calls, qualified prefixes,
single evaluation, null and bounds failures, writable views, private-child
concatenation, nested package elaboration/captured body locals, visible private
discriminants and class-wide use-type equality. Four workers repeat the checks
with independent state. Whole-program, per-unit and deterministic IR modes pass.
`supportedrejectionerrors` checks genuine call/index ambiguity, wrong index
rank/type, named indices, constant function-result components, hidden body
locals/full-view fields, private limited views and genuinely limited arrays.
Compiler ASan/UBSan checks pass for both fixtures and all four promoted cases.

## Deferred prerequisites

The remaining **24 legal C rejections** have these concrete prerequisites.
These categories reflect explicit subset boundaries; implementing a prerequisite
must be followed by a fresh review and execution of the entire compound case.

| Cases | Prerequisite | Roadmap owner |
| --- | --- | --- |
| C35003A | Array range attributes in scalar subtype range constraints | P8 discrete-range syntax |
| C36104A, C36104B, C37002A | Subtype-mark discrete ranges in index constraints/aggregate choices; range-attribute constraints | P8 discrete-range syntax |
| C36180A | Range attributes as array-declaration indices | P8 discrete-range syntax |
| C41207A | Range-attribute choices in array aggregates | P8 discrete-range syntax |
| C43204A | Array bounds beginning with function calls/qualified expressions | P8 discrete-range syntax |
| C37003A, C37006A | Discriminant constraints on access subtypes' designated records | P8 access-subtype constraints |
| C3A0007 | Record extension of a private tagged ancestor | P6f private tagged views |
| C432001, C432004, C760009 | Private extension declarations and their aggregate/finalization interactions | P6f private extensions |
| C460006 | Abstract primitive subprogram declarations | P6f abstract subprograms |
| C540001 | Generic formal derived types and their primitive operations | P6g formal derived types |
| C640001, C760010 | Generic formal tagged types | P6g tagged generic formals |
| C54A03A | Predefined ASCII.NUL | P7 predefined library |
| C760011 | New discriminants on a derived controlled tagged type; independent REPORT.Ident_Char dependency | P6f discriminated tagged types; P8 REPORT adapter |
| C761010 | Unknown-discriminant private views | P6f private views |
| C432005, C761001 | Task entries/accept bodies; protected/task aggregates and finalization interactions | P9 full tasking |
| C85005A, C85005B | Task entry renamings, accept/select rendezvous and task definitions | P9 full tasking |

The **seven harness-blocked cases** retain separate prerequisites. Missing
support is not replaced with stubs, and none is promoted to PASS.

| Cases | Support and independent language prerequisites | Roadmap owner |
| --- | --- | --- |
| C35502C, C41104A, C54A07A | REPORT.Ident_Char | P8 REPORT adapter |
| C64005C | REPORT.Ident_Char and predefined ASCII | P8 REPORT adapter; P7 predefined library |
| C761004 | TCTouch.Flush and three-argument Validate, with real trace semantics | P8 TCTouch adapter |
| C940005, C940010 | Target-configured ImpDef; compound tests also exercise task allocation and full protected entry/body behavior, including internal operations, queuing and exception propagation | P8 ImpDef configuration; P9 full tasking |

All 20 B observations remain ungraded. Their error markers and legal controls
still require the Phase 8 grader; a compiler rejection does not establish a
legality pass. No parser cascade from an explicitly unsupported prerequisite is
classified as an independently confirmed defect without a focused reproducer.
