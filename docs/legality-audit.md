# Non-tasking legality findings from B-test grading

The six Phase 8 targets now pass unchanged under location-aware grading:
B33101A, B34001B, B34002B, B37004A, B460001 and BC60001. The selected B cohort
has **17 PASS and nine FAIL**. Remaining full-source failures have task,
protected-access or concurrent-interface prerequisites owned by Phase 9.
No upstream source or required error site was removed.

## Compiler repairs

Known discriminants on scalar, enumeration, array and access definitions are
rejected under [RM 3.7(8)](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-7.html).
Derivation from a scalar with a new discriminant part gets a legality diagnostic,
rather than an unrelated implementation-limit diagnostic. Permitted record,
private and tagged-derived declarations keep their existing behavior; new
discriminants on an untagged derived record remain explicitly unsupported.

Access-discriminant conversions use the accessibility of the enclosing object
view under [RM 3.10.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-10-2.html),
including renaming and implicit dereference. A statically deeper source is
rejected under [RM 4.6(24.17)](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-6.html).
A local pointer variable whose access type has a sufficiently long lifetime
remains legal: the variable's own declaration level does not replace the
access type's level for a dereferenced object. Access-parameter levels retain
runtime checks. The existing coextension escape test now forwards through an
access parameter so that it continues to exercise an actual dynamic check;
new rejection controls cover direct static escapes and object renaming.

A record's name inside its definition denotes the current object, so it cannot
be the subtype mark of an allocator ([RM 8.6(17)](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-8-6.html)).
The distinct exception for anonymous access definitions remains legal. The
positive control also found that folding the current object's Size during
record layout captured the extent of only the fields placed so far. Size now
uses the existing deferred evaluation path after layout is complete. A
variable-length record checks that its default field equals the final object's
Size, including from four tasks.

Null defaults on formal functions and abstract formal subprograms are parsed
into the existing formal representation and rejected during semantic analysis.
Their diagnostic locations still point at the null token. This keeps invalid
generic declarations separate from later contexts, letting the existing
Intrinsic-convention Access prohibition be checked in the same source. No
invalid formal is emitted, and no intrinsic callback rule is relaxed.

## Attribute and diagnostic review

Aft, Fore, Small and Delta on integer/enumeration types now produce fixed-point
prefix-category diagnostics. Legal fixed-point uses remain executable controls.
Emax, Epsilon, Large, Mantissa, Safe_Emax, Safe_Large and Safe_Small are Ada 83
compatibility attributes, not Ada 2005 requirements; their use gets an explicit
language-target diagnostic. They are documented by GNAT as
[implementation-defined compatibility attributes](https://docs.adacore.com/gnat_rm-docs/html/gnat_rm/gnat_rm/implementation_defined_attributes.html).
This does not add floating-point compatibility extensions or claim Annex G.

The grader accepts the exact two-line declaration span in B33101A and the
existing intrinsic-Access message in BC60001. B37004A already diagnosed illegal
component references and mismatched allocator types. Its policy now recognizes
the allocator mismatch at line 75, the repaired current-instance check at line
77, and explicitly reviewed secondary diagnostics from unresolved component
names. Those recovery messages are restricted by source, line and message;
they cannot satisfy the required undeclared-component diagnostic. The invalid
component constraints/defaults are checked independently of their recovery.
No unmarked or OK-line errors were allowed.

## Evidence and boundaries

`legalityaudit` executes legal discriminated records, recursive anonymous access,
final object Size, fixed attributes, ordinary callback actuals and null defaults,
same-level/nested access conversions, object aliases and implicit pointer
dereferences. It runs whole/per-unit/deterministic compilation and four workers.
`discriminantcategoryerrors`, `currentallocatorerrors`,
`discriminantconversionerrors`, `numericattributecategoryerrors` and
`genericnulllegalityerrors` pin rejection diagnostics and semantic recovery.
The existing generic-null and coextension regressions remain in validation.

All 145 raw baseline cases were re-probed: only the six targeted B diagnostic
streams change. C outcomes stay at 112 PASS, six legal-source rejections and
one known runtime failure; raw B observations remain separate from grades.
All 149 upstream source hashes are unchanged. Release validation covers
1,932 tests, with the generic abstract-null diagnostic fixture refreshed for
its newly reached independent profile error. Twenty-one distinct focused
compiler/runtime ASan/UBSan checks pass, including the six promoted B-tests,
all-case grading stability, rejection controls, four workers, whole/per-unit
compilation and determinism. This increment does not implement tasking prerequisites, untagged
record discriminant remapping, Ada 83 attribute extensions or general ACATS
conformance.
