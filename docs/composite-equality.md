# Composite equality and unbounded strings (Ada 2005 4.5.2)

Ada.Strings.Unbounded, Wide_Unbounded and Wide_Wide_Unbounded now compare
logical contents consistently in scalar calls, enclosing records/arrays and
generic formal types. Independently allocated equal strings compare equal;
mutation, unequal lengths and unequal characters produce inequality.
Unchanged ACATS CXAC004 passes its complete stream/file REPORT scenario.

## Equality selection

[RM 4.5.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-5-2.html)
defines composite equality using primitive equality for tagged components and
predefined equality for untagged components. This depends on the component's
type, regardless of whether the enclosing value is an array, a tagged record,
or an untagged record. Type extensions continue to use primitive equality for
their parent part. Tagged primitive exceptions propagate through ordinary calls.
The comparison order and permitted early termination remain unspecified.

The emitter now applies that rule consistently through compareObjects. It
previously bypassed tagged primitive equality in arrays and untagged enclosing
records, while incorrectly selecting user-defined equality for untagged record
components inside tagged records. Ordinary untagged user-defined equality still
does not replace predefined equality inside composites or private generics.

RM 4.5.2(32.1) also requires equality of nonlimited language-defined types to
behave as predefined equality for composites and generic formals. Each unbounded
string remains an untagged private record containing tagged controlled Storage.
Storage now has primitive equality comparing the designated character arrays,
with null storage treated as empty. String-array equality compares positions,
so allocation addresses and different internal lower bounds do not matter.
This representation satisfies the library requirement through the ordinary
tagged-component rule, without special-casing library names in the emitter.

The public unbounded/unbounded equality overload delegates to Storage equality.
It reads the existing arrays instead of allocating temporary string copies.
The byte, wide and wide-wide families use the same strategy, preserving native
character widths. Initialization, deep-copy Adjust, finalization and streaming
formats retain their existing ownership and layout.

## Evidence

`unboundedequality` checks all three families with separately allocated strings,
Unicode values, shifted internal/outer bounds, empty strings/null arrays, nested
and tagged records, arrays of records, multidimensional arrays, generic private
formals, copies, mutation and length differences. The main task and four workers
repeat the scenarios. `textstreamsvalues` now checks whole-array equality after
both per-element Read and array Input, alongside per-component content checks.

`componentequality` checks primitive tagged equality in arrays and nested records,
multidimensional traversal, generic formals and exception propagation. Its
untagged primitive deliberately raises if called; enclosing tagged/untagged
records, arrays and private generics must use predefined equality instead.
Both new programs have whole/separate/deterministic controls. Existing array,
record, tagged, generic and string regressions remain active.

CXAC004's original source and hashes are unchanged. Its
[manifest](../tests/acats/text_streams/manifest.json) now records PASS;
`acats.cxac004` requires the complete successful REPORT output, zero exit status
and empty stderr. The former known-failure execution guard is replaced by this
regular executable C-test and a source-hash check. Selected ACATS evidence does
not establish conformance.

Validation on Darwin arm64: Release 1,901/1,901 tests, 14 focused ASan/UBSan
checks and six generated-Ada TSan checks pass. All 145 original backfill
outcomes are unchanged; only the compiler fingerprint is refreshed. ASan/UBSan
instrument compiler/runtime code; generated QBE accesses are not ASan-instrumented.
