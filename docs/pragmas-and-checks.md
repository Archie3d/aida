# Pragmas and checks — Phase 6h

The language target is Ada 2005 RM [2.8](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-2-8.html),
[11.4.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-11-4-2.html),
[11.5](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-11-5.html)
and [6.5.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-6-5-1.html).
Phase 6h implements assertion policies, check permissions and nonreturning
procedures for the compiler's supported types and profiles. This completes the
milestone with the explicit foreign callback restriction below; it does not
claim support for every language-defined pragma.

## Assertions

`pragma Assert (Check [, Message])` is executable in declarative parts and
statement sequences. Check must have a Boolean type and Message must have type
String. Positional and named associations retain Check, Message order. An
enabled assertion evaluates Check once, evaluates Message only on failure and
raises `Ada.Assertions.Assertion_Error`. Condition/message exceptions propagate
normally. Failure unwinds controlled objects through the usual exception path.

`pragma Assertion_Policy (Check | Ignore)` is a configuration pragma, as in
Ada 2005. The implementation default is Check. Ignore omits both expressions
at execution but still checks their legality. A generic retains the policy at
its definition, independently of the instantiation's policy. Configuration
history survives bundled units, exported units, separate bodies and subunits.
Ada 2012 local and assertion-kind policies are diagnosed.

`Ada.Assertions` provides Assertion_Error and both standard Assert procedures.
Calls to those procedures remain enabled under Ignore. The compiler loads
the package implicitly for assertion pragmas. Exception_Name for this predefined
exception is `ADA.ASSERTIONS.ASSERTION_ERROR`.

## Check permissions

Suppress and Unsuppress accept one of the twelve standard check identifiers
or All_Checks, in configuration/declarative/package positions. A permission
applies from the pragma through the end of its region; nested changes do not
leak into an enclosing region. Unsuppress revokes the corresponding permission.
Unknown check names, extra arguments and statement placement are diagnosed.

The emitter can omit its scalar range, access null, record discriminant,
array length and elaboration checks. It conservatively retains checks across
package, generic and subunit boundaries, including separately compiled code.
Checks performed inside C runtime helpers remain enabled, including indexing,
arithmetic, accessibility, allocation, storage and tag checks. Other checks
can also remain where emission uses a checked helper. Ada permits retaining
checks despite Suppress; this pragma grants permission to omit checks rather
than guaranteeing their removal. No unsafe execution after a suppressed
failure is promised. Static legality, assertions and No_Return behavior remain
independent of All_Checks.

## Nonreturning procedures

No_Return accepts local procedure and generic-procedure names, including
overloads. Functions, null procedures, instances and nonprocedures are
diagnosed, as are return statements applying to a nonreturning procedure.
Overrides must repeat No_Return; renamings-as-body require a nonreturning
target. The checked generic contract carries this property into instances.

A nonreturning Ada body that completes normally cleans up its locals and
propagates Program_Error to the caller. Its own handlers cannot intercept
that completion failure. Explicit exceptions retain their original identity.
Direct, renamed, generic, dispatching and indirect Ada calls use this behavior.
Imported procedures also receive a guard at direct calls, including bare calls
without an argument list. Taking 'Access of an imported No_Return procedure
is diagnosed: a foreign callback guard adapter remains part of the foreign
ABI work in Phase 7. Taking 'Access of a No_Return renaming of an ordinarily
returning target also requires a guard adapter and is diagnosed pending the
Phase 8 callable-completeness audit. Direct calls through that renaming are
guarded, without changing calls to its target. Ordinary imported callbacks
retain their existing support.

## Advisory pragmas and remaining owners

List(On/Off), Page and Optimize(Time/Space/Off) validate their arguments and
have no execution effect. The compiler generates no listing, and all Optimize
choices use its existing QBE pipeline. Controlled requires a nonderived access
first subtype declared in the same region and rejects duplicate specifications.
It has no execution effect: this runtime does not collect or move objects
while they still exist. Finalization and collection destruction at the access
type's master still occur. See RM
[13.11.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-11-3.html).
Inline remains advisory. Existing
Import/Convention, elaboration and Atomic/Volatile support is unchanged.

Pure/Preelaborate and Preelaborable_Initialization are enforced by
[Phase 7 categorization checks](package-categorization.md). Phase 8 owns packing and the
remaining representation/legality inventory; Phase 9 owns concurrent policy
pragmas, including task Storage_Size. The implementation-defined supported
restriction set for Restrictions is empty; every restriction pragma is
diagnosed, and no Annex D/H restriction profile is claimed. Other unsupported
pragmas continue to diagnose.
The completion checkbox does not move these existing roadmap obligations.

## Validation

`pragmapolicies`, `pragmaignore`, `pragmalibrary_main`, `pragmaunitbundle`,
`pragmanoreturn`, `pragmasuppressed` and `pragmaunsuppressed` run whole-program,
per-unit and determinism checks. Policy/library and nonreturning scenarios
include four workers. `runtime.pragmaimport_modes` links a returning foreign
procedure to check the required failure in both compilation modes.
`semantics.pragmapolicies` inspects emitted IR for omission and restoration
of range/null checks, using only valid executions in suppressed regions.
Six focused legality fixtures reject bad arguments, placement, ignored but
ill-typed assertions, invalid nonreturning declarations, invalid Controlled
types and foreign 'Access.
Unchanged upstream C651001 executes with the full successful REPORT protocol.
The pinned CB42001–CB42003 assertion sources use Ada 2012 local policies and
are excluded from this Ada 2005 selection; no assertion-specific upstream
PASS is claimed.

Validation on Darwin arm64: 1,408/1,408 CTest cases pass, including 38 focused
pragma/upstream checks. All 129 frozen baseline observations are unchanged.
The eight whole/per-unit execution scenarios, six rejection fixtures and
C651001 also pass with an AddressSanitizer/UndefinedBehaviorSanitizer compiler
and C runtime. Generated Ada memory accesses remain uninstrumented, and
LeakSanitizer is unavailable on this host.
