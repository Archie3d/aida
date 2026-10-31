# Final Phase 7 wide-library audit

The roadmap's wide-character/string increment is complete for AIDA's documented
Ada 2005 subset. This closes its remaining attribute profiles, hash functions
and cross-family audit; it is not a claim of complete compiler or predefined
library conformance. The existing [foundation](wide-foundation.md),
[string facilities](wide-strings.md), [wide I/O](wide-text-io.md) and
[exception](exceptions-library.md) contracts retain their representation and
host choices.

## Inventory and legality

| Ada 2005 area | Implemented interface and category | Evidence |
| --- | --- | --- |
| A.3.1 | Empty Ada.Wide_Characters and Ada.Wide_Wide_Characters roots, Pure | `widehashes`, `widewidehashes`, Pure `WideHashCategories` |
| A.3.4, J.14 | Native conversion overloads and Characters.Handling compatibility views, Pure | `widefoundation`, unchanged CXA3004 |
| A.4.1, A.4.7–8 | Wide_Space/Wide_Wide_Space; maps, constants, fixed, bounded and unbounded families, Preelaborate with private initialization promises | Both widths' `*stringmaps`, `*fixedstrings`, `*fixedstringchecks`, `*boundedstrings`, `*unboundedstrings`, `*stringlimits`, `*stringcategories` |
| A.4.7–9 | Root/fixed hash functions, Pure; generic bounded and unbounded hash functions, Preelaborate; narrow counterparts use the same algorithm | `stringhashes`, `widehashes`, `widewidehashes`, Pure client and category rejections |
| A.11 | Wide/Wide_Wide_Text_IO, six nested numeric/enumeration generics, standard numeric instances, bounded/unbounded text adapters | `wideiolibrary`, `widewideiolibrary`, both `*textstringio`, unchanged CXAB001 |
| 3.5, 3.5.1, 8.5.4, 12.6 | Wide_Image/Wide_Value and Wide_Wide_Image/Wide_Wide_Value as renamings, renamings-as-body, named/defaulted/boxed generic actuals; base scalar profiles and native string types | `wideattributes`, `wideoverloads`, `wideprofileerrors`, unchanged C354003 |
| 11.4.1 | Wide exception-name overloads and Unicode expanded names | `exceptionlibrary`, `exceptionlibrarytasking`, `unicodeunitbundle` |

The scalar attribute-subprogram profile builder is shared by generic actuals
and renamings. It covers integers, modulars, real and fixed types, enumerations,
and both native character types. Wide text is not silently narrowed. Renaming
a subtype's Value returns the base scalar type, and its Image accepts base
values outside the subtype's constraint. Width attributes denote values, not
functions. Record Image remains outside the Ada 2005 target; taking Access of
an intrinsic attribute renaming remains illegal.

`wideoverloads` imports both map/bounded families together and checks contextual
literals, overloaded values/results, boxed overloaded generic defaults and an
attribute renaming-as-body. Rejections pin incompatible narrow/wide profiles,
non-subprogram attributes, non-scalar prefixes, intrinsic Access, callback
accessibility, genuine literal ambiguity and forbidden Pure dependencies. A similarly named predefined hash alias to
another target is also rejected, pinning the narrow scope of the parent exception.
The prior ownership, boundary, callback-exception and four-task checks remain
part of this audit, rather than being replaced by hash/profile tests.

## Hash definition and categorization

A.4.9 allows an implementation-defined hash. AIDA uses the FNV-1a recurrence
with a 32-bit modular accumulator, offset 2166136261 and multiplier 16777619.
Each step xors the **whole character position** and then multiplies modulo
2**32. There is one step per character, independent of string bounds, native
cell width and UTF-8 encoding. Empty strings retain the offset value. No Unicode
normalization or case folding is performed. The same positional sequence has
the same hash through the root, fixed, bounded and unbounded views.

The internal Pure generic `Ada.Strings.Generic_Hash` defines this recurrence
once. Public root functions are its instantiations; fixed children are
renamings, and bounded/unbounded children hash the corresponding string value.
The bounded hash's formal package is the nested Generic_Bounded_Length instance;
it does not require the still-open general generic-parent/child loader feature.

The parser accepts the short own-unit name in a categorization pragma after a
child subprogram, as used in the standard declarations. A.4.9 explicitly
specifies Pure for Fixed.Hash even though Fixed is Preelaborate; A.4.7/8 carry
that declaration into both wide families. The loader confines that predefined
exception to the three exact root-hash renamings and their implicit parent
edges. Other context dependencies and user units keep the normal categorization
checks. Bounded/unbounded hash children remain Preelaborate.

## Supplementary enumeration text

Wide_Image's documented twelve-cell spelling `["00010400"]` for an
unrepresentable identifier character is now decoded by Wide_Value before
Unicode identifier validation/case comparison. Native Wide_Wide_Image/Value
continue to use the actual code point. Tests round-trip supplementary Deseret
identifier names, lowercase escape input, all supported scalar families and
highest character/modular values. Malformed/truncated/out-of-Unicode-range
escapes raise Constraint_Error through normal Value validation. Width tests
account for the escaped Wide_String representation.

## Validation and remaining owners

`wideattributes`, `wideoverloads`, `stringhashes`, `widehashes` and
`widewidehashes` run whole-program, per-unit and deterministic-emission checks.
Four workers exercise attribute wrappers and independent hash/container callers.
Hash tests cover shifted/Integer'Last bounds, empty strings, known ASCII
results, high character bits, all wrappers and actual use as an
Indefinite_Hashed_Maps hash callback. WideRuntimeTest adds an escaped-image
conversion round trip to its exhaustive/four-thread checks.

Unchanged C354003 passes for wide modular attributes, alongside the earlier
CXA3004, CXA4012–CXA4023, CXA4025, CXA4029, CXA4033, CXA4034 and CXAB001
cases. The pinned 3.1 archive contains no dedicated hash case; project
regressions supply that coverage. Provenance is in the
[ACATS inventory](../tests/acats/README.md).

The full regression run passes 1,678/1,678 checks; the three added overload
checks and the predefined-parent rejection also pass. Focused whole/per-unit
compiler/C-runtime ASan/UBSan checks pass, along with C-runtime TSan for the
four-worker attributes and exhaustive C codec test.

The existing signed-32-bit public string/result limits, sparse-map construction
complexity, Unicode 4.0 source policy and strict UTF-8 external-file choices
remain documented implementation choices. Broader privacy/freezing/overload
coverage, full ACATS/B-test grading, native cross-host execution and the general
generic-parent/child loader remain Phase 8. Text_Streams children belong to the
Phase 8 predefined-environment/stream-I/O audit, outside these A.4/A.11 string
families; specialized Complex_IO/Editing children retain their Annex owners.
Ada 2012 Wide_Characters.Handling and Wide_Wide_Characters.Handling are not part
of this Ada 2005 target. Generated QBE memory accesses remain uninstrumented by
the C-runtime sanitizers, and host LeakSanitizer is unavailable.
