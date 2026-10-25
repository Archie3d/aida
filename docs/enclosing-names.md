# Expanded names of enclosing declarations

A prefix may name a package or an enclosing block, loop or subprogram
([Ada 2005 RM 4.1.3(10–13)](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-4-1-3.html)).
For example, `Region.Value` denotes the declaration immediately within Region,
even when an inner declaration hides Value. Qualification does not search use
clauses or outer regions for a missing selector. Ordinary prefix hiding still
applies. Multiple enclosing callable constructs denoted by the prefix make it
ambiguous independently of the selector.

Body scopes are registered before their declarations are analyzed, so local
initializers, nested profiles/defaults and nested bodies can name enclosing
objects, types, overloads and exceptions. A non-package prefix is usable only
from within its region; a subprogram or generic renaming does not qualify that
region. Package-body locals remain inaccessible outside the body. Statement
identifiers remain invalid as values, and ordinary labels cannot qualify locals.
Loop parameters can be selected through their enclosing loop name and remain
constant. Nested statement names retain their declaration in the containing
body/block even when loops or conditionals intervene.

Generic formals are selectable through the current generic instance from
inside its region. They are not exported as declarations of an instance.
Contract recording/replay preserves the prefix and final declaration identity
when instantiation renames the unit; dotted type/exception names use the same
scope rules. Selected captured variables use the existing static links and
storage, without evaluating a construct prefix as a value.

The shared lookup handles expressions, calls, type/exception names and
operators. Package-body operator calls also retain access to declarations in
the package specification. Task/protected naming retains the existing subset;
full tasking and broader current-instance rules remain in Phases 8–9.

`enclosingnames` checks hidden locals/parameters, profiles/defaults, overloads,
qualified types/exceptions, mutable records/arrays, captured callbacks, nested
blocks/loops, generic formals/body locals and package-body locals/operators.
It executes whole/per-unit builds with deterministic compilation and repeats
from four tasks. `enclosingnameerrors` checks bare statement values, plain and
non-enclosing labels, nonlocal selectors, renamed callable prefixes, overloaded
enclosing callables, constant loop parameters and hidden package/generic data.
Compiler ASan/UBSan checks cover both fixtures and unchanged ACATS C83022A,
C83024A, C64004G, C64005A and C64005B. All five execute as PASS in the strict suite.
