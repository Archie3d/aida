# AIDA Ada compiler
![build](https://github.com/Archie3d/aida/actions/workflows/build-and-test.yml/badge.svg)

This project implements Ada compiler in C++. It uses [QBE](https://c9x.me/compile/) as a backend.

> This is an experimental project built with a help of AI.

The compilation is performed in three stages:
- Ada to QBE compiler frontend, which translates Ada source code to QBE intermediate language (IR).
- QBE backend, which compiles IR to the target's assembly.
- Target's assembler and linker combines the QBE's output with the language runtime to produce an executable.

A compiler driver is provided. This executes all the steps of the compilation to go from Ada source code to an executable.

## Building

```shell
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
The build also compiles the vendored `qbe` submodule into `build/qbe/qbe`, and
the C run time into `build/runtime/libadart.a`.

The main CTest suite includes executable Ada regressions, compiler diagnostic
checks, a QBE IR comparison, and runtime and driver checks. Ada fixtures and
their `.expected` files live in `tests/ada/` and are registered in
`tests/CMakeLists.txt`. To run a single case, use, for example,
`ctest --test-dir build -R '^ada.sliceassignment$' --output-on-failure`.

### Building on Windows
When compiling on Windows use [Msys2](https://www.msys2.org/) environment.
You will need `gcc` (mingw), `make`, and `cmake` installed.

## Installing

```shell
cmake --install build --prefix /usr/local
```

which lays the compiler out the way GNAT lays itself out, the names saying what
each directory holds:

| Path | Contents |
| --- | --- |
| `bin/ada`, `bin/adac`, `bin/qbe` | The driver, the front end and the backend |
| `lib/ada/adainclude/` | The predefined environment as Ada source |
| `lib/ada/adalib/libadart.a` | The C run time every program is linked against |

An installed `ada` works out where it stands, following the symbolic link it
was reached through and searching `PATH` when it was named without a directory,
and reads the environment from `../lib/ada/adainclude` beside itself. So the
tree can be moved after installing, and nothing has to be pointed at by hand.

## Using the compiler

`ada` is the driver. It runs `adac` to produce QBE IL, `qbe` to translate that
IL into assembly and `cc` to assemble and link the program with the run time
library. Executable builds retain per-unit IR, assembly, objects and dependency
records in `<program>.adaobj`, or the directory selected with `-D`.

```
ada [-o <program>] <source> [<source>...]
```

| Option | Effect |
| --- | --- |
| `-o <file>` | Name of the produced file |
| `-D <dir>` | Directory for cached per-unit build artifacts |
| `--clean` | Remove the selected artifact directory and stop |
| `--emit-ir` | Stop after generating QBE IL |
| `-S` | Stop after generating assembly |
| `-k`, `--keep` | Keep the IR file when using `-S`; executable builds always retain their cache |
| `--no-stdlib` | Leave the predefined environment out of the unit search path |
| `-v` | Print each command before running it |
| `-h`, `--help` | Print available options |

Unchanged units reuse their compiled artifacts. Changes to a specification
invalidate its clients; changes to an ordinary body rebuild that unit only.
Generic bodies also invalidate units that read them for instantiation. A changed
front-end executable invalidates its compilation records automatically, as do
missing or modified IR files. Older record formats are rebuilt automatically.

Specifications and bodies have separate elaboration entry points, so another
unit can elaborate between them while their code still shares one object file.
The binder uses the elaboration order recorded by the loader in the scan
manifest; it does not independently solve elaboration constraints. It checks
compiled records against that manifest and rejects missing or stale units and a main unit without a main procedure. When invoking the front
end manually, run `--scan` again after source changes before binding.

The driver tells `adac` where the predefined environment is, so nothing needs
naming on the command line. `--no-stdlib` withholds it, and a program that
withs a predefined unit is then rejected with `cannot find the unit`; the run
time library is linked either way, since a program raising `Constraint_Error`
calls into it whether or not it names a predefined unit.

The `ADAC`, `QBE`, `CC`, `ADA_RUNTIME` and `ADA_LIBRARY` environment variables
override what the driver picks up, and have the last word over anything it
works out for itself.

`adac` itself is a front end only: it reads Ada source files and writes QBE IL,
and never runs another program.

```
adac [options] <source> [<source>...]
```

| Option | Effect |
| --- | --- |
| `-o <file>` | Name of the produced file, `-` for standard output |
| `-D <dir>` | Directory for per-unit output and metadata |
| `-c` | Compile the last named unit against its dependencies; requires `-D` |
| `--scan` | Write the dependency and elaboration manifest; requires `-D` |
| `--bind <unit>` | Validate compiled units and generate the entry point; requires `-D` |
| `-I <dir>` | Another directory to look for units in; takes priority in `-c` mode |
| `--stdlib <dir>` | Where the predefined environment lives |
| `--no-stdlib` | Leave the predefined environment out altogether |
| `-h`, `--help` | Print available options |

Only the main procedure needs to be named. Every unit a `with` clause mentions
is looked for on the library path and compiled along with it, so a package
specification and its body are found by their file names rather than listed.
Several sources may still be given at once. For executable builds, the last
source names the unit containing the parameterless main procedure.

### Separate compilation and binding

Binding is automatic for `ada` executable builds; there is no separate `ada`
bind option. With the installed compiler on `PATH` and a main procedure in
`hello.adb`, this builds the program and prints the scan, compile, bind, and
link commands:

```shell
ada -v -D hello.adaobj -o hello hello.adb
```

The front end's explicit bind command can then be run against those artifacts:

```shell
adac --bind hello -D hello.adaobj
```

`hello` is the unit key without its source extension. Binding reads
`units.manifest` and the units' `.ali` records, validates compiler/source/IR
consistency against the manifest, and writes `__ada_binder.ssa`. That generated
native `main` calls each specification/body elaboration routine in order, then
the Ada main procedure. An unhandled exception stops execution with status 1.
Binding alone does not run QBE, assemble, or link.

For manual builds, use `adac --scan -D <dir> <main.adb>`, compile each unit in
that manifest using `adac -c -D <dir> <unit-source>`, then bind. Supply the same
search paths for each invocation. Scanning still parses the source closure;
compilation reads dependency specifications and any bodies needed for generic
instantiation. `.ali` files are dependency records, not serialized interfaces,
so dependency sources are still required. Ada `separate` subunits remain
unimplemented.

`ada --emit-ir` and `ada -S` use whole-program output, including the generated
entry point, rather than the per-unit cache. Use `adac --help` and `ada --help`
to inspect the installed compiler's options. To remove the program's cached
artifacts, use `ada --clean -D hello.adaobj hello.adb`; the driver currently
requires a source argument even with `--clean`.

## Layout

| Path | Contents |
| --- | --- |
| `adac/` | The compiler: lexer, parser, semantic analysis and QBE code generation |
| `ada/` | The driver, dependency checks and object cache that coordinate `adac`, `qbe` and `cc` |
| `common/` | Shared content-digest support |
| `runtime/ada/` | The predefined environment, written in Ada |
| `runtime/adart.c` | Run time support (`'Image`, `'Value`, real number formatting, array comparison, raising and reporting exceptions) |
| `runtime/adaio.c` | The file layer behind `Text_IO`, `Sequential_IO`, `Direct_IO` and `Stream_IO`; compiled into `libadart.a` |
| `runtime/adanumerics.c` | Floating-point numerics helpers, also compiled into `libadart.a` |
| `tests/ada/` | Ada test programs with their expected output |
| `tests/golden/` | Recorded QBE IL used to notice code generation changes |
| `qbe/` | The QBE backend, as a submodule |

## Supported language

Objects, constants and named numbers; integer, floating point, Boolean,
character, enumeration, array, record, access and private types; discriminants
and variant records; subtypes with range and discriminant constraints;
expressions including `mod`, `rem`, `**`, `&` and
short circuit operators; `if`, `case`, `while`, `for`, plain loops with `exit`,
blocks; procedures and functions with `in`, `out` and `in out` parameters,
recursion and nested subprograms with up level references; package
specifications and bodies with elaboration code; the `'First`, `'Last`,
`'Length`, `'Range`, `'Pos`, `'Val`, `'Succ`, `'Pred`, `'Digits`, `'Width`,
`'Image`, `'Value`, `'Address`, `'Size`, `'Read`, `'Write`, `'Input` and
`'Output` attributes; a `'Size` representation clause; `pragma Import`; generic
packages and subprograms with their instantiations; run time range checks
raising `Constraint_Error`, `raise` statements and exception handlers.

Object renaming gives an existing object another name, including record fields:

```ada
X : Integer renames R.Field;
```

The alias shares storage with the field: assigning through either name changes
what the other reads. Nested fields, indexed components, access values, package
objects, and aliases used by nested subprograms or `in out` parameters are
supported. The target is evaluated once when the declaration is elaborated.
A field of a constant remains read-only through its alias, and the alias inherits
the target's subtype constraints. Runtime-bound array aliases currently require
a subprogram scope, as do runtime-bound array objects.

Integer types declared with `type T is range L .. H` and their subtypes are
checked wherever a value crosses into them: on assignment, on initialisation,
on argument passing, on conversion and on a returned result.

`Integer` uses 32-bit signed storage and `Long_Integer` uses 64-bit signed
storage. User-defined integer ranges exceeding 32 bits also use 64 bits;
subtypes preserve their parent's storage width. Integer arithmetic checks
addition, subtraction, multiplication, division, negation, `abs`, and `**`
for machine-range overflow. Division, `rem`, and `mod` check zero divisors.
These failures raise `Constraint_Error`, as do out-of-range numeric conversions.
The checked operations currently call C runtime helpers, which adds call overhead.

Modular integer declarations (`type Byte is mod 256;`) support static integer
moduli from 1 through `2 ** 32`, including nonbinary moduli. Addition,
subtraction, multiplication, negation, exponentiation, and the logical operators
`and`, `or`, `xor`, and `not` use modular semantics. Division, `mod`, and `rem`
check zero divisors; negative exponents raise `Constraint_Error`. Constant
folding and runtime evaluation share the same overflow-safe arithmetic.
`'Modulus`, scalar subtypes, comparisons, conversions, and loops are supported.
Conversions and subtype boundaries check ranges; `'Succ` and `'Pred` do not wrap.

Modular types currently use 32-bit storage when their full range fits a signed
32-bit value, and 64-bit storage otherwise. Supported `'Size` clauses can select
8 or 16 bits when the modulus fits; a modulus above `2 ** 31` requires 64-bit
storage. Full unsigned 64-bit modular types, modular generic formals, and
`Ada.Text_IO.Modular_IO` remain future work. The modular regression programs
cover arithmetic, static values, logical operations, boundaries, and diagnostics.

Ordinary fixed-point types use signed 64-bit scaled integers:

```ada
type Voltage is delta 0.125 range -100.0 .. 100.0;
Reading : Voltage := 1.25;
Adjusted : Voltage := Reading * 3;
```

The supported `delta` range is `2.0**(-30)` through `1.0`. The compiler chooses
`Small` as the largest power of two no greater than `delta`, with 0 through 30
fractional bits. Bounds and values must fit signed 64-bit scaled storage.
Decimal and based real literals and static real expressions use checked exact
128-bit rational evaluation when converted to fixed point. Expressions beyond
that evaluator's capacity are diagnosed. A constant expression is converted
as a whole; floating-point operands first retain their own precision.

Supported operations include addition, subtraction, unary signs, `abs`,
comparisons, multiplication/division by integers, and fixed-by-fixed products
and quotients with a fixed-point result context. Explicit conversions between
fixed-point, integer, and floating-point types check the target range. Scaling
rounds to nearest, with halfway results away from zero; compile-time folding
and runtime arithmetic share the same checked scaling routines. Overflow,
division by zero, and failed range checks raise `Constraint_Error`.

Static fixed-point subtype bounds, scalar parameters and returns, arrays,
records, and `'First`, `'Last`, `'Base`, `'Small`, `'Delta`, and `'Size` are
supported. Storage is currently always 64 bits. Decimal fixed point, explicit
`Small` clauses, dynamic fixed-point subtype constraints, fixed-point formal
type declarations, fixed-point I/O/streaming, and additional fixed-point
attributes remain future work. Exponentiation, `mod`, and `rem` are not
predefined fixed-point operations.


`Long_Integer` is supported by `'Image`, `'Value`, generic `Integer_IO`, and
`for` loops. A loop tests its final value before incrementing or decrementing,
so a loop ending at a machine limit does not wrap around.

Calls with argument lists, bare function names, and package-selected names use
the expected result type when selecting an overload, and report ambiguity when
several profiles remain. A bare call may omit all arguments when every formal
has a default. Procedure call statements select procedures, and enumeration
literals use the expected enumeration type. Nested calls retain candidate types
until all arguments and the surrounding result context select a profile. For
example, in `Consume (Pick, True)`, the second argument can select a `Consume`
overload whose first formal then determines which `Pick` is called. This context
also flows through arithmetic, comparisons, unary operators, array indexing,
record selection, and user-defined operators. Named arguments
cannot be repeated or followed by positional arguments. Ambiguous nested calls
remain errors even when their possible results have the same type.

Omitted default expressions are evaluated at each call, including imported
calls, with names bound in the declaration scope. Each parameter in a grouped
profile gets its own default evaluation. Explicit arguments skip their defaults;
default evaluation can raise an exception before the called body is entered.
Only `in` parameters may have defaults, and the default must have a compatible
type. Defaults use the same value conventions as explicit arguments.

Record and constrained-array function results use caller-provided storage.
Unconstrained-array results preserve their bounds and are copied into the caller
before use, including in nested calls, indexing, attributes, and constrained
object initialization or assignment. Length mismatches raise `Constraint_Error`;
null results preserve their bounds and have zero length. Functions of every
result type that fall through without returning raise `Program_Error`, including
when a handler finishes without returning a value.

Variable-size results and concatenations use a caller-owned temporary list,
reclaimed at statement boundaries and after repeated while-condition and array
fill evaluations. Returned buffers are adopted without another stack copy.
Unconstrained aggregates infer their bounds: positional notation starts at the
index subtype's lower bound, and named notation uses its choice bounds. This
works for local initializers, call arguments, qualified expressions, and function
returns, including single dynamic range choices and null ranges. Bounds are
evaluated once; component expressions run once per element. `others` still needs
bounds from the surrounding context.
The new internal return convention requires rebuilding Ada code and using the
matching runtime; imported C calls retain their existing convention.

Handler choice parameters (`when E : ...`) are local constants of the limited
private type `Ada.Exceptions.Exception_Occurrence`. Each binding retains the
caught identity and message through nested handlers and calls, and can be captured
by a nested procedure or passed to an `in` parameter. `Ada.Exceptions` provides
`Exception_Identity`, both `Exception_Name` overloads, `Exception_Message`,
`Exception_Information`, `Null_Id`, `Null_Occurrence`, `Raise_Exception`, and
`Reraise_Occurrence`, and both `Save_Occurrence` overloads. Exception names support
`'Identity`.

`raise E with Message;` evaluates a `String` message before raising. Messages
retain all bytes, including embedded NULs, through handler entry and re-raising.
Inspection returns strings with lower bound 1. Names currently contain the
uppercase defining identifier. `Exception_Information` starts with the name and
optional `: ` plus message, followed by the original raise location and an Ada
call traceback. Locations use source-file basenames, line numbers, and columns;
traces contain up to 32 compiled Ada subprogram/elaboration frames, innermost
first. Foreign/native frames are not recorded. Saves and re-raises preserve the
original diagnostic snapshot. An allocation failure while copying a message
instead reports a new `Storage_Error` at the failing call.
Null-occurrence inspection and null-ID raises follow the
[Ada.Exceptions rules](https://ada-rapporteur-group.github.io/ARM/Ada_2012/RM-11-4-1.html).
Unhandled reports include the message; `Exception_Information` supplies the
additional location and traceback. `Save_Occurrence (Target, Source)` retains
up to the first 200 message bytes in the target, using Ada's permitted truncation;
it requires no allocation. `Save_Occurrence (Source)` returns an
`Exception_Occurrence_Access` preserving the full message in one allocation.
Both copies outlive the source handler. The access result can be freed with
`Ada.Unchecked_Deallocation`; replacing its contents through the procedure still
leaves a single allocation to free. The access type uses the currently supported
pool-specific subset rather than general `access all` semantics.
Occurrence streaming and wide names remain unimplemented; occurrence-valued function
results are diagnosed. The occurrence layout changed, so previously compiled Ada
objects require rebuilding against the matching runtime.

Within block, subprogram, and package-body handlers, bare `raise;` re-raises the original
exception, even after a nested handler or called routine handles a different
exception. Bare raises outside handlers or inside an enclosed body are rejected,
following the [Ada raise-statement rules](https://docs.adacore.com/live/wave/arm22/html/arm22/RM-11-3.html).
Separate handlers cannot cover the same exception, including through renaming;
repeated choices within one handler are allowed. `others` must stand alone in
the final handler, and exception parts and handlers cannot be empty.
`Numeric_Error` is an alias of `Constraint_Error`.
An unhandled exception during library elaboration is reported with exit status 1
before the main procedure is called. Library package declarations and body
statements execute in declaration order within the loader's unit order, including
nested packages and library generic instances. Package-body handlers can recover
and let elaboration continue; declaration failures bypass that package's handlers,
and failures raised by a handler propagate outward.

Packages declared inside subprograms and their blocks elaborate whenever
execution reaches the declaration. This includes generic instances: each call
gets fresh local state, recursive calls keep independent state, and package
routines can access enclosing variables through static links. Package bodies
and handlers execute at that point, including when the enclosing routine is
called during library startup. Existing generic formal-parameter restrictions
still apply.

Specification/body elaboration can be interleaved across units, using the
loader's dependency order. General elaboration-before-use checks, remaining
visibility/cycle cases, and declarations inside library-level statement blocks
still need work.

Statically constrained multidimensional arrays support nested aggregates,
checked indexing on every axis, assignment, equality, and subprogram parameters
and results:

```ada
   type Matrix is array (2 .. 3, 5 .. 7) of Integer;
   M : Matrix := ((1, 2, 3), (4, 5, 6));
   -- M (3, 7) = 6; M'Length (2) = 3
```

`First`, `Last`, `Length`, and `Range` accept a static dimension number (default
1). Dimensions may use enumeration indices or null ranges.

Unconstrained multidimensional types support static subtype constraints and
runtime constraints on local objects:

```ada
   type Matrix is array (Integer range <>, Integer range <>) of Integer;
   subtype Small is Matrix (1 .. 2, 1 .. 3);
   M : Matrix (1 .. Rows, 5 .. Columns + 4) := (others => (others => 0));
   Copy : Matrix := M;
```

Every dimension travels with unconstrained parameters and function results,
including through nested subprograms. Assignment checks all dimension lengths
and slides values to the destination bounds. Equality compares shapes and
components, including the remaining dimensions when an outer dimension is null.
Bounds use 32-bit indices; runtime dimension lengths are limited to `Integer'Last`,
and storage-size overflow raises `Storage_Error`.

Multidimensional aggregates can infer their bounds in initializers, arguments,
and function returns:

```ada
   type Matrix is array (Positive range <>, Positive range <>) of Integer;
   A : Matrix := ((1, 2, 3), (4, 5, 6));
   B : Matrix := (2 .. Rows + 1 => (5 .. Columns + 4 => 0));
```

Positional dimensions start at the index subtype's lower bound when there is
no target constraint; named dimensions use their choices. String literals can
supply character rows. Index choices are evaluated once before component values.
Corresponding subaggregates must have identical bounds, including null ranges;
otherwise `Constraint_Error` is raised. `others` still requires an applicable
target constraint. These checks follow the
[Ada aggregate rules](https://www.adaic.org/resources/add_content/standards/22rm/html/RM-4-3-3.html).

Local named array subtypes and constrained array types can also use runtime
bounds, in one or multiple dimensions:

```ada
subtype Buffer is String (2 .. N);
type Grid is array (1 .. Rows, 4 .. Columns + 3) of Integer;
A : Buffer := (others => ' ');
B : Grid := (others => (others => 0));
```

Bounds are evaluated and checked once when the declaration is elaborated. Later
changes to `N`, `Rows`, or `Columns` do not change those bounds. Each call or block
re-entry gets its own bounds, also available to nested routines and local packages.
Aliases retain the original bounds. Type/subtype `'First`, `'Last`, `'Length`, and
`'Range` work without an object; object declarations reuse the saved constraints.
Parameters and results check dimension lengths and slide to the declared bounds.
Declaring a type allocates no array data; each dimension is limited to
`Integer'Last` elements, with larger lengths raising `Storage_Error`.

Library-level runtime array declarations, components and allocators using
runtime-constrained array subtypes, and `'Size` and streaming for those subtypes
remain unsupported and diagnosed. Stream attributes for unconstrained
multidimensional arrays also remain unsupported.

Local integer and enumeration subtype declarations can use runtime ranges,
including 64-bit `Long_Integer` bounds:

```ada
subtype Index is Integer range 2 .. N;
subtype Alias_Index is Index;
type Vector is array (Index) of Integer;
X : Index := N;
A : Vector := (others => 0);
```

Each elaboration saves the bounds once; aliases share them and subsequent changes
to `N` do not alter the subtype. Bounds survive nested calls, local packages,
recursion, and block re-entry. Initialization, assignment, conversions, qualified
expressions, scalar input arguments/defaults, and function results use the saved
constraints. `'First`, `'Last`, `'Range`, membership, and loops use them too;
`'Base` retains the underlying type's unconstrained scalar range. Array types and
aggregate inference can use these subtypes as indices, within the existing
32-bit array descriptor limits.

A non-null range must fit its parent subtype; a null range is allowed even when
its bounds lie outside that subtype, following the
[Ada scalar range rules](https://www.adaic.org/resources/add_content/standards/95lrm/ARM_HTML/RM-3-5.html).
Failures during declaration elaboration propagate to the enclosing handler.
Runtime bounds and constants of runtime subtypes cannot stand in for static
case choices or integer type bounds.

Runtime scalar constraints currently require a named local subtype declaration.
Anonymous constraints, library declarations, runtime real subtypes, `'Width`, and
streaming for runtime scalar subtypes remain diagnosed as unsupported.

Ada scalar `out` and `in out` parameters have separate value storage. An `in out`
parameter copies and checks the actual value against the formal subtype before
the call; numeric and enumeration `out` parameters start uninitialized. Access
`out` parameters retain the initial access value without a constraint check.
On normal return, each value is checked against the actual object's subtype,
including saved runtime bounds, before assignment. Propagated exceptions skip
copy-back; a handled exception followed by normal return permits it. Actual
addresses are evaluated once. Copy-back uses formal declaration order, so earlier
copies can remain if a later check fails. Imported C conventions and composite
parameter mechanisms are unchanged. See the
[Ada parameter rules](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-6-4-1.html).

Local one-dimensional arrays can use runtime index constraints or take their
bounds from an initializer:

```ada
Buffer : String (1 .. N) := (others => ' ');
Copy   : String := Make_Text;
```

The object keeps its bounds for its lifetime. Assignment checks lengths and
slides the source to the target bounds; indexing checks runtime bounds. Bounds
also travel through slices, function results, calls, and captured variables.
Runtime-bounded aggregates support positional elements, a final `others`, named
index choices and ranges, and a single dynamic range choice. Named aggregates
without `others` keep their own bounds and slide to the target after a length
check. Components are evaluated per element in temporary storage before the
completed value is copied to the target, so self-references read the old value
and a failed component evaluation leaves the target unchanged. Dynamic choice
bounds are evaluated once. A dynamic or null range must be the aggregate's only
choice, following the [Ada array aggregate rules](https://docs.adacore.com/live/wave/arm95/html/arm95/arm95-4-3-3.html).
Overlapping or noncontiguous static named choices and mixed positional/named
associations are diagnosed. Component defaults are also evaluated per element;
grouped declarations evaluate an initializer separately for each object.

Dynamic local array data uses heap storage released when its block finishes,
on labelled loop exits, or when an exception leaves the scope. A block's locals
remain alive in its own handlers; enclosing objects remain alive when an inner
block fails. Function returns release remaining local and temporary allocations.
This implementation uses signed 32-bit bounds,
limits lengths to `Integer'Last`, checks allocation sizes, and raises
`Storage_Error` on allocation failure. Library-level dynamic objects remain
unsupported.

Floating point types are declared with `digits`, optionally with a range:

```ada
type Coefficient is digits 10 range -1.0 .. 1.0;
type Real is digits 8;
subtype Probability is Real range 0.0 .. 1.0;
```

Up to six digits are held in a single precision number and up to fifteen in a
double precision one, which is what the `s` and `d` types of QBE provide; more
than fifteen digits is rejected. A range is checked at the same places as an
integer one. Following Ada, the two families of numbers stay apart: a whole
number literal is not a real value, so `C : Coefficient := 1;` is an error
while `C : Coefficient := 1.0;` is not, `mod` and `rem` need integer operands,
predefined `**` takes an integer exponent, and converting a real value to an
integer rounds rather than truncates. The numerics generics also provide an
overloaded `"**"` accepting a real exponent (see Numerics below).

Operator functions support `+`, `-`, `*`, `/`, `mod`, `rem`, `**`, `&`, `and`,
`or`, `xor`, `not`, `abs`, and the six comparison symbols. For example:

```ada
function "+" (Left, Right : Vector) return Vector;
...
C := A + B;
C := "+" (A, B);
C := Vectors."+" (A, B);
```

Unary `+` and `-` take one parameter; their binary forms take two. `abs` and
`not` take one, and other operators take two. All parameters must be `in` and
cannot have defaults. Operator symbols containing letters are case-insensitive.
Explicit calls to declared operators also support named arguments.

Resolution considers visible user-defined and supported predefined profiles,
using operand and result context. A user-defined homograph replaces its
predefined operation; distinct applicable profiles remain ambiguous. A local
operator does not hide unrelated overloads brought in by `use`. Operator calls
use ordinary function calling conventions, including composite results and
exception propagation, and are not folded as predefined arithmetic.

A Boolean-valued `"="` implicitly declares the complementary `"/="`; explicitly
declaring a Boolean-valued `"/="` is rejected. Comparison operators may also have
non-Boolean result types, selected by context. `and then` and `or else` retain
short-circuit evaluation and cannot be overloaded. General derived-type
inheritance, `use type`, and the remaining visibility audits are still separate
roadmap items.

Strings are arrays of characters and carry their bounds along with the data, so
an unconstrained `String` parameter answers `'First`, `'Last` and `'Length` at
run time. Slices, `&` on strings, and comparison with `=`, `/=`, `<`, `<=`, `>`
and `>=` are supported; assigning a value of a different length raises
`Constraint_Error`.

### Composite comparisons

Array types have declaration identity: two separately declared types are
incompatible even when their bounds and component types match. Subtypes and
slices retain the parent array type, and bounds may differ when assigning or
comparing values of that type. String literals and aggregates take their type
from context; a literal matching two array overloads is ambiguous. Character
concatenation preserves the selected character array type. Explicit array
conversions are checked separately; the supported cross-type subset requires
identical component subtype objects and compatible (or integer) index types.

Array equality compares corresponding elements by position, independently of
lower bounds, and requires equal lengths. Null arrays compare equal even when
their bounds differ. Comparison is recursive for record and array elements;
floating elements use numeric equality rather than byte equality. Ordering is
available for one-dimensional arrays of discrete elements and is lexicographic.
Arrays of floating-point, record, array, or access elements support equality
but not predefined ordering.

Record equality compares common components and discriminants, then only the
active variant's components. Padding and inactive variant storage are ignored.
These rules follow [Ada 95 RM 4.5.2](https://www.adaic.org/resources/add_content/standards/95lrm/ARM_HTML/RM-4-5-2.html).

### Access types and allocators

An access type designates objects taken from storage rather than declared. A
type may be named before it is described, which is what lets a record and the
access type pointing at it be written in either order:

```ada
type Node;
type Link is access Node;

type Node is
   record
      Value : Integer;
      Next  : Link;
   end record;
```

`new Node` takes storage for one node and answers a `Link` to it; `new Node'(1,
null)` gives the new object its value at the same time, and `new String (1 ..
5)` says how long the array is to be. An allocator has no type of its own, so
what it makes is read from where it is used, the way `null` is. Storage comes
out cleared, so an access component of a new object starts as `null`, and a
`Storage_Error` is raised when the request cannot be met.

`P.all` is the object a link designates, and it may be left out in front of a
component or an index: `P.Value` and `P.all.Value` are the same thing, as are
`T (1)` and `T.all (1)`. Reaching through `null` raises `Constraint_Error`.

Storage is given back through `Ada.Unchecked_Deallocation`, a generic procedure
over the access type and what it designates:

```ada
procedure Dispose is new Ada.Unchecked_Deallocation (Node, Link);
...
Dispose (Head);    -- Head is null afterwards.
```

Nothing checks that no other access value still designates the object, which is
what "unchecked" says: reaching through one of those afterwards is the program's
own doing.

### Private types

A package may name a type without showing what it is made of. Outside the
package the name, the operations the package declares and assignment and
equality are all there is; the components, the aggregate and the predefined
operators are out of reach. A type declared `limited private` lends not even
assignment or equality.

```ada
package Piles is
   type Pile is private;
   Empty : constant Pile;
   procedure Push (P : in out Pile; Value : in Integer);
private
   type Contents is array (1 .. 8) of Integer;
   type Pile is
      record
         Items : Contents;
         Top   : Integer := 0;
      end record;
   Empty : constant Pile := (Items => (others => 0), Top => 0);
end Piles;
```

`Empty` above is a deferred constant: the visible part names it so that users
may write it, and the private part gives it a value once the representation is
known. A constant may be left without a value only there, and one never given
a value is an error, as is a private type never completed.

The full declaration fills in the very type the visible one named, so an access
type or a subprogram that mentioned it goes on meaning the same thing.

### Discriminants and variant records

Grouped record fields retain their full constraints and defaults. For example,
`X, Y : Integer range 1 .. 10 := Next_Value;` gives both fields the range and
evaluates `Next_Value` separately for each field whenever an object is created.
This also applies to array and record constraints and fields in variant parts.
Only the active variant's defaults are evaluated; exceptions stop initialization
and propagate to the enclosing handler.

A discriminant is a component named in the type declaration and fixed when an
object is declared. A variant part makes the rest of the components depend on
it:

```ada
type Figure is (Point, Circle, Rectangle);

type Shape (Kind : Figure) is
   record
      Label : Character;
      case Kind is
         when Point  => null;
         when Circle => Radius : Integer;
         when others => Width, Height : Integer;
      end case;
   end record;

subtype Round is Shape (Circle);

C : Round          := (Kind => Circle, Label => 'c', Radius => 5);
R : Shape (Rectangle) := (Rectangle, 'r', 3, 4);
```

The alternatives share their storage, so a `Shape` has one size whichever kind
it holds and an object of it never has to move. A discriminant is fixed when the
object is declared and never changes, which is why it takes no default and
cannot be assigned to; an object of the unconstrained type is rejected for
saying nothing about which variant it has.

Where nothing has fixed the discriminant beforehand, the aggregate settles it
itself, so `new Shape'(Rectangle, 'r', 3, 4)` makes a rectangle without the
subtype having to say so.

The choices of a variant part are read the same way as those of a case: static
values, ranges, `|` between them and `others` at the end, each value covered
exactly once and all of them covered. Where a subtype fixed the discriminant,
naming a component of another alternative is an error the compiler reports;
where nothing fixed it, as in a parameter of the unconstrained type, the value
carries the answer and reaching for the wrong component raises
`Constraint_Error`:

```ada
function Area (S : in Shape) return Integer is
begin
   case S.Kind is
      when Point     => return 0;
      when Circle    => return 3 * S.Radius * S.Radius;
      when others    => return S.Width * S.Height;
   end case;
end Area;
```

### Choosing between values

A case chooses on a discrete value. A choice is a single value, a range, a
subtype mark standing for the range it holds, or several of those separated by
`|`, and `others` takes whatever is left:

```ada
case Today is
   when Mon        => Compute_Initial_Balance;
   when Fri        => Compute_Closing_Balance;
   when Tue .. Thu => Generate_Report (Today);
   when Sat .. Sun => null;
end case;
```

Every choice has to be static, and between them the alternatives have to
account for every value the selector can take, each of them exactly once. The
seven days above are all covered, which is why that case needs no `others`;
leaving one out is an error rather than a case that quietly does nothing, a
value covered twice is an error, and so is a choice outside what the selector's
subtype holds. Ada leans on all of that, and so does this compiler: a case
compiles to a plain choice with no run time check behind it.

### Numbers in another base

A based literal writes its value in any base from 2 to 16, with an optional
fraction and an exponent that scales by a power of that base:

```ada
Mask    : constant Integer := 16#FF#;
Pattern : constant Integer := 2#1010_1010#;
Scaled  : constant Integer := 16#1#E4;
Half    : constant Real    := 16#F.8#;
Small   : constant Real    := 16#1.0#E-2;
```

`Integer_IO.Put` takes a `Base` alongside `Width` and writes the value back in
the same notation, so `Put (255, 0, 16)` prints `16#FF#`. `'Image` stays
decimal. Integer `'Value` and `Integer_IO.Get` also accept based numerals,
underscore separators, and nonnegative exponents, with syntax, overflow and
subtype checks. Floating-point `'Value` is not implemented.

### Numerics

`Ada.Numerics` provides `Pi`, `e`, and `Argument_Error`. Its
`Generic_Elementary_Functions` child supplies square root, logarithms,
exponential, real power, trigonometric functions (radians or an explicit
`Cycle`), and hyperbolic functions and inverses.
`Elementary_Functions` instantiates it for `Float`; `Long_Elementary_Functions`
for `Long_Float`. Other floating point types can instantiate the generic directly.

Example:

```ada
with Ada.Numerics.Elementary_Functions;
procedure Example is
    use Ada.Numerics.Elementary_Functions;
    Root : Float := Sqrt (2.0);
    Sine : Float := Sin (30.0, Cycle => 360.0);
    Cube : Float := 2.0 ** 3.0;
    Root_Again : Float := "**" (2.0, 0.5);
begin
    null;
end Example;
```

Compatibility with this compiler:

- Real exponentiation uses the standard `**` operator. Make the package
  directly visible with a use clause for infix calls, or use a qualified call
  such as `Ada.Numerics.Elementary_Functions."**" (2.0, 0.5)`.
- Profiles use `Float_Type'Base`, so a constrained actual subtype does not
  constrain arguments, results, or intermediate calculations. Intermediate
  calculations use C float for 32-bit types and C double for 64-bit types.
- Invalid domains raise `Ada.Numerics.Argument_Error`. Poles and non-finite C
  results raise `Constraint_Error`; underflow to zero is permitted.
- Calculations use the platform libm at the actual type's machine precision;
  this is not a claim of conformance to the accuracy requirements of the
  optional Numerics Annex.
- On Linux and other systems with a separate libm, link generated assembly
  with `cc program.s /path/to/libadart.a -lm -o program`. The existing Ada
  driver's link command does not pass `-lm`. On macOS its normal command works.

The C mappings are private implementation support: Ada wrappers validate domains
before calling them. They use the existing pending-exception runtime mechanism.

The existing double C entry points keep their names. Float32 counterparts add
`_f32` (for example `__ada_numerics_sqrt_f32`) and use the float libm functions.
The generic selects them with `Float_Type'Base'Size = Float'Size`; QBE folds
this static condition so the executed path performs no cross-precision casts.

## The predefined environment

Apart from `Standard` and `System`, which the compiler builds because the
language is defined in terms of them, the predefined units are ordinary Ada
source under `runtime/ada/`. They are compiled with the program that draws on
them, and nothing in them is spelled differently from what a program of your
own may write.

| Unit | File |
| --- | --- |
| `Ada` | `ada.ads` |
| `Ada.Characters`, `Ada.Characters.Latin_1`, `Ada.Characters.Handling` | `ada-characters*.ads`, `.adb` |
| `Ada.Strings`, `Ada.Strings.Maps`, `Ada.Strings.Fixed`, `Ada.Strings.Bounded` | `ada-strings*.ads`, `.adb` |
| `Ada.Command_Line` | `ada-command_line.ads` |
| `Ada.IO_Exceptions` | `ada-io_exceptions.ads` |
| `Ada.Exceptions` (inspection and raising subset) | `ada-exceptions.ads`, `ada-exceptions.adb` |
| `Ada.Text_IO` | `ada-text_io.ads` |
| `Ada.Text_IO.Integer_IO` | `ada-text_io-integer_io.ads`, `.adb` |
| `Ada.Text_IO.Float_IO` | `ada-text_io-float_io.ads`, `.adb` |
| `Ada.Text_IO.Enumeration_IO` | `ada-text_io-enumeration_io.ads`, `.adb` |
| `Ada.Integer_Text_IO` | `ada-integer_text_io.ads` |
| `Ada.Float_Text_IO` | `ada-float_text_io.ads` |
| `Ada.Sequential_IO` | `ada-sequential_io.ads`, `.adb` |
| `Ada.Direct_IO` | `ada-direct_io.ads`, `.adb` |
| `Ada.Streams` | `ada-streams.ads` |
| `Ada.Streams.Stream_IO` | `ada-streams-stream_io.ads` |
| `Ada.Unchecked_Deallocation` | `ada-unchecked_deallocation.ads`, `.adb` |
| `Ada.Numerics` | `ada-numerics.ads` |
| `Ada.Numerics.Generic_Elementary_Functions` | `ada-numerics-generic_elementary_functions.ads`, `.adb` |
| `Ada.Numerics.Elementary_Functions` | `ada-numerics-elementary_functions.ads` |
| `Ada.Numerics.Long_Elementary_Functions` | `ada-numerics-long_elementary_functions.ads` |
| `Ada.Text_IO.Scanning` | `ada-text_io-scanning.ads` (internal input helper) |

### Character, string, and command-line packages

The character and string packages are implemented in Ada, following the
Character/String interfaces in Ada RM
[A.3](https://www.adaic.org/resources/add_content/standards/12rm/html/RM-A-3.html)
and [A.4](https://www.adaic.org/resources/add_content/standards/12rm/html/RM-A-4.html). `Ada.Characters.Latin_1`
provides the ISO 8859-1 constants. `Ada.Characters.Handling` provides character
classification, case and basic-letter conversion, and ISO 646 tests and
substitution, including String overloads. These operations use Latin-1 values,
not the host locale or UTF-8 decoding. String conversions return bounds starting
at 1. Standard object-renaming aliases in `Latin_1` are equivalent constants;
`Is_Decimal_Digit` is a wrapper for `Is_Digit`.

`Ada.Strings.Maps` supplies private character sets, set algebra, range/sequence
conversion, and character mapping values. `Ada.Strings.Fixed` supplies searching,
counting, token finding, translation, replacement, insertion, overwriting,
deletion, trimming, padding, and repetition. Search results retain source indices;
function results of type String start at 1. Mutating operations validate and
prepare their result before assigning it, including when source slices overlap.

`Ada.Strings.Bounded.Generic_Bounded_Length` supplies bounded strings with an
inline buffer and length, initialized to the empty string. Its conversions,
concatenation, comparison, selection, search, and transformation operations use
the same string semantics, with the standard `Length_Error` and truncation
policies when capacity is exceeded. `Slice` preserves the requested bounds;
`To_String` starts at 1. Capacity actuals must currently be static and positive.
Generic contract analysis preserves symbolic subtype and record-component bounds
until instantiation supplies the capacity. Bounded objects own no heap storage;
some operations still create temporary String results.

```ada
with Ada.Characters.Handling;
with Ada.Strings;
with Ada.Strings.Fixed;
with Ada.Strings.Bounded;

procedure Example is
    package Names is new Ada.Strings.Bounded.Generic_Bounded_Length (64);
    Name : Names.Bounded_String := Names.To_Bounded_String
        (Ada.Characters.Handling.To_Lower
            (Ada.Strings.Fixed.Trim ("  Ada  ", Ada.Strings.Both)));
begin
    Names.Append (Name, " compiler");
end Example;
```

These remain supported subsets: wide characters/strings, mapping-function
callbacks (`Character_Mapping_Function` and their overloads),
`Ada.Strings.Maps.Constants`, and package categorization enforcement
(`Pure`/`Preelaborate`) are not provided. The implementations do not claim full
predefined-environment conformance.

`Ada.Command_Line` exposes `Argument_Count`, `Argument`, `Command_Name`, and
`Set_Exit_Status`. Arguments are the host's `argv[1..argc-1]`, excluding the
command name, with bytes unchanged and String lower bound 1. `Command_Name`
returns `argv[0]` without path normalization. Out-of-range argument numbers raise
`Constraint_Error`. `Exit_Status` ranges from 0 to 255; `Success` is 0 and
`Failure` is 1. The last status set is returned on normal termination, defaulting
to success; unhandled exceptions return 1. The binder initializes arguments
before library elaboration in both compilation modes. Rebuild against the
matching runtime, which now supplies the binder's command-line entry points.

Regression programs cover Latin-1 boundaries, mapping errors, null strings,
non-1 and maximum indices, overlapping updates, bounded capacities and copying,
and command-line arguments/status in both whole-program and separate builds.

### Library lookup

A unit lives in the file its name gives, lowered with each dot turned into a
hyphen, so `Ada.Text_IO.Integer_IO` is `ada-text_io-integer_io`. The
specification is `.ads` and the body `.adb`. In a normal front-end invocation,
search roots are the command-line source directories in order, followed by
`-I` directories, `ADA_INCLUDE_PATH`, and the bundled library. In `adac -c`
mode, explicit `-I` directories come first so the driver can preserve the scan's
search order. `ada` preserves the command-line source directories automatically;
use `ADA_INCLUDE_PATH` for additional directories when invoking the driver.
The bundled library is `runtime/ada` in the build tree and
`lib/ada/adainclude` once installed.

What these units cannot say in Ada they say with `pragma Import`, which ties a
declaration to an entry point in the C run time:

```ada
procedure Put_Line (Item : in String);
pragma Import (C, Put_Line, "__ada_put_line");
```

The pragma applies to the declaration just given, so each overload names the
routine that carries it out. Anything a package can write for itself it writes
for itself: `Integer_IO.Get` tests the range and raises `Data_Error`,
`Enumeration_IO` is built on `Enum'Image` and `Enum'Value`, and `Sequential_IO`
and `Direct_IO` hand an element to the run time as `Item'Address` and
`Element_Type'Size`, which is why one entry point serves every instantiation.

`System` declares `Address`, the type `'Address` yields, and `Storage_Unit`,
the number of bits `'Size` counts in. A size clause fixes how wide a type is
laid out, which is how `Ada.Streams` says that a stream element is a byte:

```ada
type Stream_Element is range 0 .. 255;
for Stream_Element'Size use 8;
```

### Input and output

Numbers and enumeration values are written through the generic children of
`Ada.Text_IO`, one instance per type:

```ada
package Level_IO is new Ada.Text_IO.Integer_IO (Level);
package Real_IO is new Ada.Text_IO.Float_IO (Real);
package Day_IO is new Ada.Text_IO.Enumeration_IO (Day);
```

Each instance carries the defaults Ada derives from the type it was made with,
so `Integer_IO.Put` lays out a field of `Num'Width` and `Float_IO.Put` shows
`Num'Digits - 1` places after the point, both overridable per call through
`Width`, or `Fore`, `Aft` and `Exp`. `Enumeration_IO.Put` writes the literal in
upper case unless given `Lower_Case`, and `Get` reads one back, raising
`Data_Error` on a word the type has no literal for. `Ada.Integer_Text_IO` and
`Ada.Float_Text_IO` are the instances on `Integer` and `Float`, which is what
they are in Ada, so a value of another numeric type needs an instance of its
own rather than a conversion.

`'Image` on an enumeration value gives the literal in upper case.

`Text_IO` works on files as well as on the standard ones. `File_Type`,
`File_Mode`, `Create`, `Open`, `Close`, `Delete`, `Reset`, `Is_Open`, `Mode`,
`Name` and `Form` manage a file, and `Put`, `Put_Line`, `Get`, `Get_Line`,
`New_Line`, `Skip_Line`, `End_Of_File` and `End_Of_Line` come in both a plain
form and one that names a file. `Set_Input` and `Set_Output` redirect the plain
form, and `Standard_Input`, `Standard_Output`, `Standard_Error`,
`Current_Input` and `Current_Output` name the files it would otherwise use.
The eight exceptions of `Ada.IO_Exceptions` are raised by the run time and
caught by ordinary handlers.

```ada
Create (Data, Out_File, "readings.txt");
Put_Line (Data, "first line");
Close (Data);

Open (Data, In_File, "readings.txt");
while not End_Of_File (Data) loop
   Get_Line (Data, Line, Last);
   Put_Line (Line (1 .. Last));
end loop;
Close (Data);
```

`Ada.Sequential_IO` and `Ada.Direct_IO` are generic on their element type and
read and write whole objects. `Direct_IO` additionally addresses the file by
element with `Read`, `Write`, `Index`, `Set_Index` and `Size`.

```ada
package Reading_IO is new Ada.Sequential_IO (Reading);
package Slot_IO is new Ada.Direct_IO (Slot);
```

`Ada.Streams.Stream_IO` opens a file as a stream. `Stream (File)` yields a
`Stream_Access`, and the `'Write`, `'Read`, `'Output` and `'Input` attributes
move values of the supported types across it. Streaming currently relies largely
on object representation; runtime-constrained subtypes and unconstrained
multidimensional arrays are not supported. `'Output` and `'Input` additionally carry
the bounds of an array whose type does not fix them, which is what makes
`String'Input` give back the string that `String'Output` wrote. `Read`, `Write`,
`Index`, `Set_Index` and `Size` work on the file directly in terms of
`Stream_Element_Array`.

```ada
Create (Archive, Out_File, "state.dat");
Channel := Stream (Archive);
Integer'Write (Channel, 1234);
String'Output (Channel, "hello");
```

## Generics

A generic unit written in Ada is remembered as the tokens it was written with,
and each instantiation parses them again with the formals bound to their
actuals. Formal types and formal objects with defaults are supported, in
positional or named notation.

```ada
generic
   type Element is private;
   Capacity : Integer := 4;
package Stacks is
   procedure Push (Value : Element);
   function Pop return Element;
end Stacks;

package Number_Stack is new Stacks (Integer);
package Letter_Stack is new Stacks (Element => Character, Capacity => 2);
```

Each instance has state and code of its own. A library-level instance elaborates
during program startup. An instance declared inside a subprogram or block
elaborates whenever execution reaches its declaration, with fresh state for each
activation. Scalar, array, and record generic formal objects accept runtime
actuals. An `in` formal captures an independent value once at instance
elaboration; an `in out` formal retains a view of the actual variable, including
its assignment constraints. Indexed components, selected components, and array
slices can be `in out` actuals, with their addresses and bounds evaluated once.
Defaults may refer to earlier formals and retain declaration-site name
resolution. Local unconstrained array formals retain runtime bounds across
nested calls and recursive activations, including multidimensional arrays.
Constrained array `in` formals slide bounds and check lengths at elaboration;
`in out` formals retain the actual bounds. Record copies check constrained
discriminants before modifying the destination. Access formals, limited `in`
objects requiring build-in-place initialization, and formal packages remain
unsupported.

A runtime capacity can constrain local scalar subtypes and local arrays using
the existing runtime-bound support. Runtime record layouts and library-level
dynamic arrays (including runtime-bound generic array objects) remain unsupported.

Formal functions and procedures accept named subprograms and quoted operator
actuals, including predefined operators and package-qualified user operators.
The actual is selected by parameter types, modes, and result type. Calls retain
the formal's parameter names and defaults, and nested actuals retain access to
their enclosing variables. A named default resolves at the generic declaration;
`is <>` uses the formal's name at the instantiation site.

For example, a sorting procedure can declare its comparison and array contract:

```ada
generic
    type Element is private;
    type Index_Type is (<>);
    type Array_Type is array (Index_Type range <>) of Element;
    with function "<" (Left, Right : Element) return Boolean is <>;
procedure Sort (Items : in out Array_Type);

-- After supplying the body of Sort:
type Numbers is array (Integer range <>) of Integer;
procedure Ascending is new Sort (Integer, Integer, Numbers);
procedure Descending is new Sort (Integer, Integer, Numbers, "<" => ">");
```

Formal arrays check dimensionality, constrainedness, index types and subtypes,
and component subtypes. Constrained formal array indexes must be subtype marks;
matching runtime-dependent constraints is not yet supported. Formal abstract
subprograms, null procedure defaults, and attribute or enumeration-literal
subprogram actuals remain unsupported.

Generic declarations and bodies are checked against distinct formal type
identities, including bodies supplied in separate files. Private formals expose
assignment and equality, limited private formals restrict copying, and discrete
formals expose ordering without assuming integer arithmetic. Invalid bodies are
rejected even if no instance is declared.

Instances reuse the names and operator choices resolved by the contract check.
For example, `I < J` on `Index_Type` retains its predefined ordering when both
indices and elements are instantiated with `Integer`; supplying `">"` for the
formal element comparison changes only element ordering. External names remain
bound to their declaration environment, and overloads on distinct formal types
retain their identity when actual types coincide. Instance analysis still checks
concrete layouts and the currently supported static actual constraints.

The input and output generics are instantiated the same way as any other, since
they are written in Ada like the rest of the predefined environment.
