# Serialized semantic interfaces

Per-unit compilation (`adac -c`, including the normal `ada` driver build) writes
`<unit>.asi` alongside `<unit>.ali` and `<unit>.ssa`. Clients can compile and
link against these artifacts after the dependency's Ada sources are removed.
`.ali` remains build/binder metadata; `.asi` supplies the checked declarations
and templates needed to reconstruct the semantic environment.

## Packaging and use

Build the library and its dependency closure with the same compiler used by its
clients. Package the `.asi`, `.ali` and `.ssa` files together in a directory.
Preserve the generated artifact names, including aliases for declarations in
custom-named source bundles. Native `.o` files are not required: the driver
uses packaged frontend IR and builds native objects in the client's build
directory with its configured toolchain.

For a frontend-only client:

```sh
adac -c -I /path/to/library-artifacts -D client-objects client.adb
```

For an executable, add the artifact directory to `ADA_INCLUDE_PATH` and invoke
`ada` normally. The path-list separator follows the host. `--no-stdlib` can be
used when the artifact package also contains the predefined dependency closure;
the runtime archive is still linked. Artifact directories may be relocated
without rewriting the original source paths stored for diagnostics.

Search roots retain their existing precedence. Within a root, Ada sources take
precedence over `.asi` files. An interface in an earlier root wins over source
files in later roots. Explicit source registrations take precedence over
lookup. Interfaces use flat, lower-cased unit keys (`ada-text_io.asi`) in the
artifact directory, regardless of the producer's source layout. An invalid
selected interface produces a diagnostic rather than falling through to a
later root. Missing/stale companion artifacts or dependency fingerprints are
errors. A source override that changes a compiled dependent's contract requires
rebuilding that dependent.

## Contents and import behavior

The versioned binary interface contains:

- The exact compiler executable digest and active host/CPU/pointer/endian ABI.
- Object-group and declared-part identities, source/specification digests,
  dependency edges, elaboration requirements and categorization metadata.
- Typed token replay streams for the successfully checked unit's sources and
  transitive subunits, with decoded literals, locations and configuration
  pragmas. Generic bodies and private completions remain available for replay.

The loader reconstructs declarations and runs the existing semantic analyzer
to create canonical scopes, types, layouts, private views, overloads, tagged
primitives, defaults and generic templates. This is a serialized replay
interface, not a serialized pointer graph or a parsing/analysis speedup.
Imported source streams are held in memory; their original files are not opened.
Diagnostic locations still name the producer's sources.

For this initial implementation, imports replay ordinary bodies too. Dependency
checking is therefore conservative: body changes may invalidate interface
clients even when an ordinary source-based client would only depend on the
specification. Whole-library closure publication is supported; replacing a
unit's implementation using only its previously published specification is not
a separate compilation mode provided here.

The reader checks the format, payload checksum, compiler/ABI compatibility,
bounds and token kinds, then verifies replayed declaration identities against
the unit metadata. Imported compiled records must agree with the selected
closure and their companion IR digests. Limits are 64 MiB of payload and one
million tokens and 4,096 source streams per interface. Old compiler binaries and manifest formats are
rejected; rebuild the producer and consumers together. Manifests use format 7;
`.ali` records retain format 3.

Atomic publication, recovery from interrupted writes and concurrent writers
remain the next roadmap item. Installed runtimes continue to ship hierarchical
Ada sources; this change supports packaging compiled interfaces without adding
an installation-wide precompilation step.

## Validation

`loading.semantic_interfaces` removes the producer's library sources and uses
`--no-stdlib` to require packaged predefined interfaces too. It checks executable
and standalone frontend clients, private representations and visibility,
defaults, overloads, tagged primitives, exception identity, hierarchical child
units, body-only subprograms, renamed views, custom bundles, generic/subunit
replay, deterministic serialization, relocation, cache reuse, invalidation after
republishing a body, explicit mixed bundles and source precedence. Negative controls cover incompatible formats, compilers and ABIs,
truncated/oversized token payloads, wrong declaration identities, missing
compiled metadata, changed dependencies and modified IR.

Validation on Darwin arm64 (2026-11-09): all 1,970 CTest checks passed. The
interface, hierarchical-layout and separate-binding checks passed under
ASan/UBSan; the expanded library-republication control also passed in native
and ASan/UBSan builds. Other native hosts remain covered by CI rather than this
local validation.
