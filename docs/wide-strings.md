# Wide string facilities (Phase 7)

The two native-width families implement the maps/constants, fixed, bounded and
unbounded string packages and their text-I/O adapters from Ada 2005
[A.4.7](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-A-4-7.html),
[A.4.8](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-A-4-8.html)
and [A.11](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-A-11.html).
This does not complete the overall wide-library audit.

## API and character semantics

`Ada.Strings.Wide_Maps` and `Wide_Wide_Maps` supply set construction,
canonical sorted range/sequence conversion, equality, complement, intersection,
union, symmetric difference, difference, membership and subset operations.
Mappings support construction, lookup and sorted domain/range extraction,
excluding fixed points. Unequal sequence lengths and duplicate domain values
(including repeated fixed points) raise Translation_Error.

`Wide_Maps.Wide_Constants` and `Wide_Wide_Maps.Wide_Wide_Constants` supply the
eleven Latin-1 sets and three maps, plus Character_Set and, in the latter,
Wide_Character_Set. The Latin-1 definitions reuse the narrow constants.
Non-Latin-1 characters are absent from the eleven sets and unchanged by those
three maps. These are positional character facilities, with no UTF encoding,
Unicode classification or normalization implied by their names.

`Wide_Fixed`, `Wide_Bounded.Generic_Bounded_Length` and `Wide_Unbounded`, and
the corresponding `Wide_Wide_` packages, supply the existing narrow packages'
operations using the RM's native-width types and names. This includes both
Index profiles, Count and both Translate forms taking mapping-function
callbacks. Null callbacks raise Constraint_Error even for empty input;
callback exceptions propagate, and failed translation preserves the target.
The existing string bounds, truncation, padding, search and ownership contracts
in [library collections](library-collections.md) also apply here.

`Wide_Text_IO.Wide_Bounded_IO`/`Wide_Unbounded_IO` and
`Wide_Wide_Text_IO.Wide_Wide_Bounded_IO`/`Wide_Wide_Unbounded_IO` provide the
explicit/current-file Put, Put_Line and function/procedure Get_Line profiles.
They use the [shared strict UTF-8 file implementation](wide-text-io.md).

## Storage, categorization and limits

Both map families use the shared internal `Ada.Strings.Wide_Support` unit.
Sets own encoded inclusive intervals rather than allocating a dense character
bitmap. Set algebra sweeps sorted interval boundaries, using 64-bit endpoints
so that the one-past boundary of the 31-bit Wide_Wide_Character domain is safe.
Large contiguous sets and complements need storage proportional to their range
count, independent of the greatest character value. Mappings own sorted
nonidentity pairs. Constants use static selectors and empty owned storage,
performing no allocation or user function calls during elaboration.

Storage uses the existing narrow unbounded string owner: copies are independent,
failed construction releases temporaries, and all allocations use the shared
access collections. Wide unbounded strings use independent controlled native
arrays and logical string streaming. No new C state or callback ABI is added.
Separate objects may be copied, changed and released by separate tasks;
concurrent access to the same mutable object requires caller synchronization.

Maps, constants, fixed, bounded and unbounded packages are Preelaborate.
Map/set and bounded/unbounded private types promise Preelaborable_Initialization.
Preelaborated client records and generic bounded instances exercise those
promises; Pure clients cannot depend on these weaker-category units.
The text-I/O children have their parent's ordinary category.

Storage remains bounded by Natural, address space and the existing array/result
ABI. Materializing a set's sequence requires one element per member; a complete
31-bit domain cannot fit a Natural-length string and raises Storage_Error.
Set construction and mapping construction use straightforward sorting/searches,
without a logarithmic-time or amortized-growth guarantee. Hash child functions,
remaining attribute-subprogram profiles and the final cross-family overload,
legality and ACATS audit retain the next wide-roadmap substep; generic children
of generic parents retain the existing Phase 8 loader prerequisite.

## Validation

For both widths, `*stringmaps`, `*fixedstrings`, `*fixedstringchecks`,
`*boundedstrings`, `*unboundedstrings`, `*stringmappingcallbacks`,
`*stringmappingtasking`, `*textstringio` and `*stringlimits` run whole-program,
per-unit and deterministic-emission checks. The existing narrow regressions
were adapted to the two native APIs. Additional limits tests use BMP and
supplementary literals, the last 31-bit character, full-domain complements,
sorted/merged ranges, non-unit and Integer'Last bounds, deep copies, streaming,
UTF-8 adapter round trips and preelaborated client records. Four workers also
allocate and copy sparse maps/sets independently. Rejection tests cover callback
accessibility and Pure dependencies.

Unchanged ACATS CXA4012–CXA4023, CXA4025, CXA4029, CXA4033 and CXA4034
pass. The local REPORT adapter adds Ident_Wide_Char; upstream sources and the
pinned baseline cohort remain unchanged. Provenance and hashes are recorded in
[the ACATS inventory](../tests/acats/README.md).

The full regression run passes 1,661/1,661 checks; the two subsequently added
category rejection checks and the final six boundary checks also pass. Focused
compiler and C-runtime ASan/UBSan checks pass whole/per-unit; C-runtime TSan
passes the four-worker sparse-map/string test in both modes. Generated QBE
memory accesses remain uninstrumented, and host LeakSanitizer is unavailable.
Native Windows/Linux execution retains the Phase 8 host-validation owner.
