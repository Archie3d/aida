# Dispatching constructors (Ada 2005 3.9)

[Ada.Tags.Generic_Dispatching_Constructor](../runtime/ada/ada/tags/generic_dispatching_constructor.ads)
provides the [RM 3.9](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-9.html)
generic function with abstract tagged limited private T, limited private
Parameters, and the abstract formal Constructor. Both type formals permit
unknown discriminants. Constructor takes `not null access Parameters` and
returns T; the instance takes The_Tag and Params and returns T'Class.
The unit is preelaborated and its instances have intrinsic convention.
Renaming and generic forwarding work; intrinsic subprogram Access is diagnosed.
An instance that would itself be an explicitly declared dispatching primitive
is rejected, as required by RM 3.9.2(10) for intrinsic convention.

## Selection and accessibility

Normal abstract-formal matching requires an actual dispatching primitive of T
with the specified profile. Private and derived actuals and interface ancestry
use the existing generic contract and dispatch-table machinery. The compiler
validates the predefined profile before enabling intrinsic lowering, including
when an explicit source replaces the runtime specification.

The_Tag and Params are evaluated once. No_Tag, an unrelated or abstract tag,
or a tag whose declaring master does not enclose the instance raises Tag_Error.
The check follows both parent and interface ancestry. The selected type need
not have the same accessibility level as T: a local instance may construct a
more deeply declared descendant when that descendant's master encloses it.
An outer instance cannot construct a type from a nested block or another task.
Library-level instances accept library-level descendants only.

Each instance captures its master at elaboration. Registered local tags retain
their declaring master separately from their dispatch static links. Checking
the actual master chain distinguishes nested blocks, recursive activations and
tasks; comparing lexical levels or enclosing frame addresses alone would not.
Registry lookup is synchronized, and constructor code runs outside the lock.
As specified by RM 3.9(25.3), using a tag for a nonexistent local type is erroneous;
retaining descriptor storage does not extend the type's lifetime.

## Forwarding and results

The bound primitive slot receives Params unchanged, including its dynamic
accessibility level and any unconstrained designated-array descriptor.
The_Tag selects the call even when Parameters is instantiated with T itself;
the parameter object cannot replace the supplied controlling tag. A null actual
raises Constraint_Error. Constructor exceptions propagate through
the ordinary Ada exception path.

Construction reuses controlling-result dispatch and the ordinary owned-result
ABI, including library and nested constructor bodies. Operand function calls
cannot consume the ultimate result destination. Specific/class-wide results,
discriminated objects, controlled adjustment/finalization, limited build-in-place
returns, forwarding functions, allocators and failed construction use their
existing storage arenas and finalization chains. The intrinsic does not copy
raw result bytes or introduce a separate allocation policy. Existing compiler
limits for type layouts and task/protected results remain applicable.

## Evidence

`dispatchingconstructor*` exercises private and inherited types, interface
ancestry, unconstrained Parameters, discriminated results, library/local
instances, named calls and renaming, parameter-accessibility escape rejection,
nulls, Tag_Error, constructor exceptions and operand evaluation counts.
Controlled and limited results verify final addresses and live-object counts
through forwarding, heap deallocation and exceptional cleanup. All six programs
have whole-program, separate-compilation and deterministic-output controls.
`runtime.dispatching_constructor` checks master ancestry and foreign-task
rejection with four native workers; the Ada functional test also uses four tasks.
Legality and malformed-profile controls check diagnostics. Synthetic component
finalization traces use stable helper names across compilation modes.

Unchanged Ada 2005 ACATS C390012 passes its complete REPORT scenario, including
private child types, constructor selection, allocation, and invalid tag/master
checks. Its [manifest](../tests/acats/dispatching_constructor/manifest.json)
pins original bytes and support inputs. The local TCTouch adapter adds the six
trace alternatives needed by that case; harness controls verify all alternatives,
trace reset and mismatch rejection. This is selected evidence, not a conformity
assessment.

Validation on Darwin arm64: Release 1,866/1,866 tests; 26 focused ASan/UBSan
checks and one native TSan check pass. All 145 original backfill observations
are unchanged; only the recorded compiler fingerprint is refreshed.
