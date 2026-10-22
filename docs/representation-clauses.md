# Representation clauses

Ada 2005 RM 13.1 and [13.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-3.html)
are the baseline. Phase 5 implements alignment first; enumeration mapping and
size-changing subtype/composite layouts remain roadmap items. Record clauses
follow discriminated records in Phase 6c.

## Alignment subset

`for T'Alignment use N;` supports complete, visible, untagged first subtypes
without task or protected parts. N is a static integer: 0, 1, 2, 4 or 8 storage
elements (one element is eight bits). Zero removes the subtype's alignment
promise; allocation still uses the natural alignment. Positive requests cannot
weaken natural alignment. Values above eight, non-power-of-two values, duplicate
clauses and unsupported targets produce diagnostics.

Representation items must form an adjacent group after the type declaration,
before any declaration can capture its layout. This conservative restriction
stands in for general Ada freezing. Clauses on later subtype declarations or
individual objects are unsupported, as are tagged/task/protected representations.

The layout helper propagates alignment to subtypes and derived types, array
components, record field offsets, local/captured/global objects and allocations.
Records gain tail padding to their alignment. Scalar widths remain unchanged;
requests needing scalar or array stride padding are rejected. An unconstrained
array accepts an increased alignment only when each element's size is divisible
by it. Requests after a Size clause use the selected width; a subsequent Size
clause cannot shrink storage below its specified alignment. Alignment cannot
invalidate an earlier confirming composite Size clause.

Subtype and object `'Alignment` queries accept no arguments. Object queries
return the alignment guaranteed by their subtype's allocation and evaluate the
prefix once, including function calls. Class-wide and hidden representations
are diagnosed. A zero subtype alignment is reported as zero; objects still
report their natural guaranteed alignment.

## Evidence

`alignmentclauses` checks attributes, inheritance, nested field padding, array
strides, empty composites, heap objects, captured locals, function-prefix side
effects and concurrent tasks. A C helper checks actual address divisibility.
The test runs in whole and separate compilation and checks deterministic IR.
`alignmenterrors` pins legality and subset-limit diagnostics. The existing Size,
representation rejection and ACATS suites remain regression coverage.

ACATS CD30002 is the relevant alignment case. Its Impdef/Storage_Elements support,
object clauses and multiple library units require further compiler/harness work;
see `tests/acats/README.md`. It is not counted as passing coverage.
