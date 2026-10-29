# Type-aware streaming

`Ada.Streams.Root_Stream_Type` is an abstract tagged limited type with
abstract `Read` and `Write` primitives. `Stream_Element_Array` has aliased
byte components. Attributes accept access to a root-class stream, including
`Object'Access` for a derived memory stream. Null streams raise
`Constraint_Error`; user callback exceptions propagate; premature completion
of a value raises `End_Error`.

`Stream_IO.Stream` returns a class-wide access value designating the file's
stream adapter. The adapter is owned by the runtime file slot and holds its
tag and file handle. Calls use the ordinary locked file operations. Closing
a file invalidates its stream aliases (RM A.12.1); the adapter is not an
independently owned file. The existing file-object scope-finalization audit,
positioned Read/Write overloads, Flush and Set_Mode remain library work.

## Values and attributes

* Elementary values use their type's `Stream_Size`. Supported clauses are
  static 8, 16, 32 and 64 bits; floating point and access values require their
  native width. Subtype Size clauses do not change the stream width. Integer
  representations use native byte order; enumeration representations use the
  declared codes; fixed point values use their scaled integer representation.
* Arrays stream elements in canonical order, with the last dimension varying
  fastest. Output/Input also write/read each dimension's bounds using the index
  base type. Null and shifted bounds survive a round trip. Read/Write use the
  actual array's existing bounds and stride.
* Records stream active components, skipping padding, tags, layout descriptors
  and ownership metadata. Read/Write include defaulted discriminants;
  Output/Input also include nondefaulted discriminants. A defaulted record read
  decodes into owned temporary storage before assignment, checking constrained
  destinations before changing their discriminants.
* Read, Write, Input and Output attribute definition clauses select Ada
  subprograms with checked profiles. Clauses after freezing, duplicate clauses,
  abstract routines and inappropriate limited-type attributes are diagnosed.
  Untagged derivation inherits specified attributes. Tagged extensions call the
  parent's Read/Write, then stream their extension components.
* Default Input performs normal initialization and owns controlled/dependent
  temporaries through the existing finalization machinery. User-defined Read
  sees an initialized result. Controlled components use their attributes.
* Class-wide Read/Write dispatch through the object's tag. Output writes the
  external tag as String'Output before the concrete Output; Input resolves it
  through Descendant_Tag and calls the concrete Input. Tags retain static links
  for local types. Unresolvable tags raise Tag_Error; abstract tags are rejected.
  Output checks the tag's accessibility level against the prefix type.

Stream attributes can be renamed and supplied as generic subprogram actuals
with their standard root-class access profiles. Intrinsic attribute 'Access
retains the compiler's existing intrinsic-subprogram restriction.

Unbounded strings and all twelve container families provide logical Read/Write
routines. Containers stream Count_Type length followed by element Output values
(and key Output values for maps), preserving indefinite bounds. Reads build a
controlled temporary, then Move it into the destination. Writes protect cursors
and elements from callback tampering; reads protect the destination while
callbacks build the temporary. Cleanup and unlock paths run on exceptions.

## Compatibility and remaining limits

The format is implementation defined and intended for matching AIDA type
contracts on a matching target. It is not a portable schema or an untrusted-data
protocol. Access values retain their process-local native representation.
Existing files made by the former raw-composite implementation are incompatible:
record padding/tags/descriptors are no longer payload, and Integer array bounds
now occupy four bytes rather than eight. Rebuild every compiled Ada unit after
the tag-descriptor ABI change.

Default streaming of access-discriminated types is diagnosed; specific types
can provide explicit attributes. Class-wide hooks for those types are unavailable.
Earlier compiler restrictions on type declarations, dependent controlled arrays,
shared scalar escaping and intrinsic subprogram Access still apply. Sequential_IO
and Direct_IO retain their separate element-representation implementations.
General file lifetime work and the full predefined-environment audit retain their
later roadmap items.

## Evidence

`streammemory` covers a nested memory stream, custom scalar attributes,
untagged inheritance, renaming, generic actuals, null streams and EOF.
`streamrecords` covers dependent discriminants, constrained copy-back and
controlled Input; `streamclasswide` covers local tag dispatch and parent custom
attributes; `streamsizes` checks stream widths and padding-independent payloads.
All four run whole/per-unit/determinism checks. `streamcontainers` round-trips
all twelve families and unbounded strings, including indefinite keys/elements,
and has a determinism check. `streamerrors` records legality diagnostics.
`runtime.stream_dispatch` checks C callback ABI, static links, invalid callback
results, exceptions, tag headers and four independent concurrent streams.
Existing scalar, enum, fixed, decimal, multidimensional and dynamic component
stream regressions remain enabled.

Unchanged pinned ACATS CDD1001 (aliased elements) and CDD2B01 with FDD2B00
(elementary Stream_Size) execute as PASS. CDD2001 was assessed but is not graded
as a pass: its dependent-component aggregate comparison raises Constraint_Error.
CDD2A01/02 remain blocked by existing conversion-view assignment and derived
record/aggregate restrictions. Those whole-source gaps are not counted as passes.

The contracts follow Ada 2005 RM [13.13.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-13-1.html),
[13.13.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-13-2.html)
and [A.12.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-A-12-1.html).
