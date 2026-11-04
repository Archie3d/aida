# Foreign results and callback guards

## Controlled results

Declare the import with `pragma Import (Ada, Function_Name, "link_name")`.
The linked function must implement AIDA's Ada ABI on the same target, with
matching type layouts, parameter modes and runtime. An AIDA-compiled
library-level function with the matching profile supplies that entry point.
Link its object and arrange elaboration of its defining unit before calling
it. The external name alone establishes neither dependency nor elaboration
order.

Direct calls, Ada callbacks and tagged dispatch use the ordinary Ada result
protocol. A composite result uses a hidden result argument before the explicit
parameters. For a controlled result, the next arguments identify the caller's
finalization owner and allocation arena. A constrained untagged result uses
a destination address, an unconstrained array uses an
[array result descriptor](array-descriptors.md), and a tagged or class-wide
result uses a pointer slot. Tagged and limited results also receive a flag
indicating whether the caller has supplied storage. The ordinary Ada ABI
adds accessibility metadata where the profile requires it. The imported
entry has no static link, even if its declaration is nested.

The callee constructs the result under the supplied owner and arena. It
registers constructed parts and rolls back a failed construction using
AIDA's usual finalization protocol. The caller checks for a pending Ada
exception before consuming the result, and its cleanup handles any result
already registered when an exception propagates. The caller performs any
subsequent Ada copy and adjustment and finalizes its owned result. Limited
results use the final destination when supplied, including through a forwarding
function. A pointer to foreign storage alone cannot satisfy this contract.

C-convention imports of controlled results receive a diagnostic. Predefined
Text_IO borrowed handle views retain their specialized runtime adapter.
The AIDA protocol is internal to the matching compiler/runtime build.
An AIDA-compiled adapter can call C to obtain the underlying resource and
construct the Ada result with its normal lifetime hooks.

## No_Return callbacks

An imported No_Return procedure's `'Access` designates a generated guard
with the corresponding Ada or native C callback ABI. The guard forwards
the original arguments and preserves any exception already pending in the
current Ada task. A normal return sets Program_Error. Renamings of a guarded
import share its callback identity. A renaming that itself adds No_Return to
an ordinary import has its own guard.

Library declarations emit stable guard entries for separately compiled
clients, so linking the defining unit requires the guard's foreign symbol.
For a native C callback, the guard returns through C with the Ada exception
pending. The next checked Ada boundary propagates it. C code between the
callback and that boundary still executes under ordinary C control flow.
This uses the existing scalar and named-access C callback profiles and
assumes a call on a thread running in the AIDA runtime.

## Validation

`importedcontrolled` exercises direct, indirect and dispatching calls,
class-wide and unconstrained results, build-in-place address identity and
rollback after a failed adjustment. `runtime.pragmaimport_modes` checks
direct and indirect guards, a C caller of the native callback, Ada
unconstrained parameters, exception preservation, late No_Return and
callback identity across library units. Both run in whole and per-unit
compilation, with deterministic IR checks.
