# Driver no-op builds and cleanup

An unchanged `ada` invocation returns before launching the frontend, QBE or C
compiler: there is no scan, bind, native compilation or link. `-v` reports
`is up to date (no-op build)`. This applies to executables and file outputs from
`--emit-ir` and `-S`. IR directed to standard output is emitted every time.

## No-op contract

A successful build saves an atomic, checksummed `build.receipt` in the artifact
directory. Before using it, the driver checks:

- Ordered source, external object/archive and runtime arguments; output mode,
  output path, build directory, library selection, target ABI and working directory.
- Selected tools, parsed sources, imported interface artifacts and required
  frontend/native/binder products, including the final output.
- The exact source/interface lookup candidates consulted by the frontend,
  including absent higher-priority candidates. A newly shadowing file invalidates
  the receipt; an unrelated directory entry does not.
- The environment, hashed in sorted order except for shell bookkeeping `_` and
  `SHLVL`. Only the digest is persisted, never environment values.

File fingerprints include content, canonical target and permissions. Touching a
file without changing its bytes does not force a rebuild. Replacing a tool,
retargeting a source/tool symlink, changing an input or removing/corrupting a
required product does. `-v` does not affect reuse. `--force` bypasses this
whole-build shortcut while retaining normal per-unit and native cache checks.

The frontend captures digests from the bytes it actually consumed. File outputs
are tied to staged output bytes, rather than blessing a different writer's
replacement. Inputs are rechecked before publication; a complete, stable artifact
set is required before saving a reusable receipt. Invalid or incomplete receipts
fall back to the normal build pipeline. File modes acquire both their output
and artifact directory guards in canonical order, reusing one guard when the
directories coincide. The inherited
locks cover receipt checks/publication and cleanup; see
[artifact publication](artifact-publication.md).

This is conservative dependency tracking, not a sandbox for external tools.
Unreadable/unknown fingerprints and opaque runtime linker options disable the
shortcut. The driver recognizes its standard `-pthread`, `-lm` and sanitizer
options and fingerprints explicitly named runtime archives. Undeclared wrapper
configuration and implicit system toolchain/SDK dependencies are outside the
snapshot; use `--force` or rebuild the artifact directory when those change.

Manifests use format 9 to carry input fingerprints. Compiled `.ali` records stay
at format 4 and native `.build` records at format 2. Older manifests require a
fresh scan; receipts are an optional optimization, not portable build packages.

## Cleanup without sources

Cleanup selects a directory before locating any compiler, backend or runtime:

```sh
ada --clean -D build/program
ada --clean -o program
ada --clean main.adb
```

`-D` selects the exact directory. Otherwise the original `-o` argument plus
`.adaobj` is used, or the last source's basename without its extension plus
`.adaobj`. Supply the original output stem when the platform adds `.exe`.
Cleanup without any selector is diagnosed rather than choosing a default.

Cleanup waits for active users of the same artifact directory and removes the
selected tree, including abandoned staging files and receipts. Missing targets
are a successful no-op; filesystem failures are reported. Regular files, the
current directory and its ancestors are refused. A selected directory symlink
is unlinked without following its target. Links nested inside the selected tree
are also removed as leaves; on Windows this includes directory junctions and
other reparse points. The persistent sibling lock guard is retained. Sources and executables outside the selected directory are preserved.

## Validation

`driver.noop_cleanup` checks subprocess elision and unchanged artifact identities,
source/content/shadowing changes, environment and argument order, runtime/external
objects, tool replacement and symlink retargeting, missing/corrupt artifacts,
corruption during linking, all file-output modes, stdout streaming, source-free
cleanup, symlink boundaries and cleanup waiting for live native compilation and assembly generation.

Validation on Darwin arm64 (2026-11-09): all 1,972 CTest checks passed. The
no-op/cleanup, artifact-publication, semantic-interface, hierarchical-layout and
separate-binding checks also passed under ASan/UBSan. Native Windows and Linux
validation remains for their CI; this local run is not cross-host evidence.
