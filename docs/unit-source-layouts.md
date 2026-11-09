# Unit source layouts

AIDA looks up a lower-cased unit name at two exact paths under each search root:

| Unit | Flat specification | Hierarchical specification |
| --- | --- | --- |
| Ada.Text_IO | `ada-text_io.ads` | `ada/text_io.ads` |
| Tree.Branch.Leaf | `tree-branch-leaf.ads` | `tree/branch/leaf.ads` |

Bodies use `.adb`. Subunits use the same mapping for their full parent and
subunit name. No recursive directory scanning occurs. Partially flattened
paths such as `tree/branch-leaf.ads` are not lookup candidates.

Each root is checked before the next root, independently for specifications
and bodies. Either layout in an earlier root wins over both layouts in later
roots. Distinct files at both candidate paths in the selected root produce a
conflict diagnostic; aliases resolving to the same file are accepted. A spec
and its body can use different layouts. Automatically selected files must
actually declare the requested unit and part; a misnamed file is an error,
not a reason to fall through to another root.

Use `adac -I <root>` or `ADA_INCLUDE_PATH` to supply library roots. The `ada`
driver uses `ADA_INCLUDE_PATH` (the host path-list separator applies). Supply
the root above the first unit component, even when compiling a child source
explicitly. Existing source-directory, command-line, environment and predefined
library precedence is retained. Discovered subdirectories are not added as
search roots. Explicit sources and bundles retain their
existing registration precedence and custom-filename extension.

Conventional hierarchical sources use the full dash-separated unit name for
object-group keys, so `tree/leaf.ads` and `other/leaf.ads` produce distinct
`tree-leaf.*` and `other-leaf.*` artifacts in the build directory. Manifests and
cache digests retain the actual source paths, including subunit dependencies.
Custom-named sources and bundles continue to use their source-group identity.
Clients may also use [serialized replay interfaces](semantic-interfaces.md)
from artifact directories. `.ali` files remain build metadata.

`loading.hierarchical_layouts` covers mixed/deep layouts, directory precedence,
conflicts, aliases, declaration identity, equal leaf names, explicit targets,
whole/per-unit IR agreement, cache reuse, subunit invalidation and generic
template body invalidation.
`host.install_relocation` verifies that the installed runtime preserves the
repository hierarchy, then checks relocated driver/frontend discovery and
overrides. The runtime sources in `runtime/ada` use this hierarchical layout;
root units and compatibility aliases remain at the root. The 186 moved source
files retain their contents. These are implementation-defined source
conventions, not an Ada conformance claim.

Validation on Darwin arm64 (2026-11-08): the build and all 1,969 CTest checks
passed, including the layout regression, relocated install, robustness and
selected ACATS controls. Native execution on other supported hosts remains
covered by their CI runs rather than this local validation.
