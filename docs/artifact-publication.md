# Artifact publication and concurrent builds

Compiler-owned file outputs are staged in unique sibling directories before
being atomically replaced. Failed or interrupted writes cannot truncate a
previously published IR file, interface, record, object, assembly file or
executable. The same mechanism is used for frontend outputs, copied library
artifacts, native tool outputs and cache records. Standard output remains a
stream rather than a file transaction.

## Publication and recovery

A per-unit frontend build stages its IR, every interface alias and its compiled
record before publishing any of them. It publishes `.ssa` and `.asi` members
first and the checksummed `.ali` commit record last. That record names and hashes
the complete interface alias set and IR. Declaration indexes live in
`.aida-interfaces`, keyed by specification/body identity, separately from the
object-group namespace. This covers split spec/body groups and group names
which collide with another group's declared unit name. Cache reuse, interface import and
binding verify every member, not just the presence of a primary interface.
Missing, truncated, stale or mismatched members invalidate the group.

A multi-file group is not one filesystem rename. A process killed between
member replacements can leave an incomplete group, which readers reject. A
subsequent source build regenerates it. A concurrent interface consumer either
uses a consistent snapshot or reports changed/stale inputs and requires a retry;
it does not silently combine different publications. Native `.build` records
bind both IR and object content to the selected tools and are published after
the object. A killed compiler/linker only writes its staging path; it cannot
publish through a terminated driver.

Normal exits remove private staging directories. An abrupt termination can
leave `.aida-tmp-<pid>-<counter>` directories; they are never considered cache
artifacts and may remain until cleanup. Their unique names prevent reuse by
later invocations. `--clean` removes staging files inside the artifact directory
along with the cache. These guarantees cover process failures and termination;
power-loss durability is not claimed.

## Writer coordination

Invocations sharing a canonical artifact directory serialize through an OS-owned
lock. The persistent guard file is a hidden sibling of that directory, so
cleaning the directory does not unlink the live lock. POSIX uses an inherited
`flock` descriptor; Windows uses an inherited exclusive file handle. The driver
holds exclusion across scan, frontend compilation, binding, native compilation
and linking. Standalone `adac -D` commands use the same lock. File-only frontend
and IR/assembly driver commands coordinate through their output directory.
Symlink and dot-segment directory aliases resolve to the same guard.

Frontend subprocesses borrow the driver's live handle through the internal
`--artifact-lock` argument, verified against the selected directory. Tool
subprocesses also inherit it, so terminating a driver does not admit another
writer while a child tool is still active. The kernel releases exclusion when
the last holder exits; there is no stale PID-file timeout or manual unlock step.
Frontend wrappers must preserve the inherited handle when forwarding the
internal argument (for example, use `exec`).

Different, non-overlapping artifact directories remain independent. When independent builds
write the same executable path, each replacement is complete; the final
successful replacement wins. A build directory is a coordinated shared cache,
not a guarantee that several programs' manifests remain installed together.

## Dependency and input validation

Manifests record the ordered search roots selected by the scan. Per-unit
subprocesses replay exactly those roots, without adding the target's subfolder
or reinterpreting the environment. This prevents separately compiled units from
resolving dependencies differently from the initial scan.

Records store canonical source provenance separately for specifications and
full units. Identical text found at a different selected source path invalidates
the affected contract. Ordinary body edits at the same paths still leave
specification-only clients reusable; generics and interface replay retain their
body contracts. Relocating a compiled interface package preserves its producer
source identities.

Cache/binder validation derives the required semantic closure from the fresh
part graph, including implicit I/O exceptions, parents, limited contexts,
shared source bundles, generic bodies and recorded interface replay contracts.
The recorded dependency keys, roles, digests and provenance must match that
closure exactly. Checking only that old dependencies still exist is insufficient:
missing/new/removed entries cannot be accepted as a cache hit.

Source digests are captured from the bytes actually parsed. The frontend checks
those inputs and the exact imported artifact bytes again before publication.
A changed or removed parsed input is diagnosed instead of attaching its new
filesystem digest to old generated code. Imported counterparts stay within
the checked artifact closure, including custom `.ada` source groups; later
source roots cannot take over an imported specification or body. Internal
object-group references use primary group artifacts rather than guessing an
Ada declaration name from a custom source filename. Imported IR, interfaces and compiled
records are copied through a checked staging snapshot into the client's cache.
Workspace edits and producer publications are not globally locked transactions;
a build whose inputs change can fail and be retried.

Manifests now use format 9 with a mandatory completion marker. Compiled records
use format 4 with a checksum and strict field/framing checks. Native cache
records use format 2. Previous formats are rebuilt or rejected. Source paths
and object-group keys retain spaces without changing their identity.

## Regression coverage

`driver.artifact_publication` exercises corrupted/missing records, objects and
interfaces; incomplete bundle alias sets; failed commit-record replacement;
checksummed records with omitted dependency entries; truncated manifests;
partial compiler/linker output; shared-directory drivers and standalone
frontends; a terminated driver whose child still holds the lock; source edits
between scan/compile and after parsing; identical-text source shadowing;
removed dependencies; nested source-root shadowing; and several programs
concurrently sharing a cache. Source-free checks also exercise split spec/body
object groups and colliding group/declaration names; unchanged ACATS CA110232 and
CA120012 retain their cross-file limited-context controls. Executables from failed builds retain their
previous behavior, and successful builds execute the expected new code.

The driver now skips scan, bind and link when its checked whole-build receipt
is current. Source-free cleanup uses the same exclusion protocol; see
[driver builds and cleanup](driver-builds.md).

Validation on Darwin arm64 (2026-11-09): all 1,971 CTest checks passed. The
publication, source-free interface, hierarchical-layout, separate-binding,
CA110232 and CA120012 checks also passed under ASan/UBSan, including process
termination, split-group aliases and later-source shadow controls. Windows and
Linux implementations remain subject to their native CI validation; this local
run is not cross-host evidence.
