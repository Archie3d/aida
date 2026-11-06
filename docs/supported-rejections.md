# Remaining supported-area rejection audit

This follows the original 6f-prelude review of the pinned 106-case baseline.
The [current ACATS baseline](acats-baseline.md) contains 129 cases. It is a review of this selection, not a
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

C460006 subsequently passed unchanged with P6f abstract operations and the
record-extension aggregate repair; it now joins the strict executable selection.

## Deferred prerequisites

The refreshed cohort has **12 legal C rejections**. Removing a prerequisite
requires a fresh execution of the complete unchanged source. The discrete-range
cases now pass; [current observations](acats-baseline.md) include earlier
REPORT adapter improvements without changing adapters in this increment.

| Cases | Prerequisite | Roadmap owner |
| --- | --- | --- |
| C35502C, C54A03A, C64005C | Predefined ASCII names | P8 predefined environment |
| C37003A, C37006A | Designated-record discriminant constraints on access subtypes | P8 access-subtype constraints |
| C761010 | Access-to-array subtype constraints and dependent controlled arrays | Existing access/controlled-array limits |
| C432005, C761001 | Task entries/accept bodies and aggregate/finalization interactions | P9 full tasking |
| C85005A, C85005B | Task entry renamings and accept/select rendezvous | P9 full tasking |
| C940005, C940010 | Missing scheduling services in partial ImpDef; independent protected/tasking prerequisites | P8 ImpDef configuration; P9 full tasking |

The remaining **harness-blocked case**, C761004, needs TCTouch.Flush and the
three-argument Validate overload with actual trace semantics (P8 TCTouch
adapter). It is not promoted to PASS.
