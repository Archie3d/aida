# AIDA Ada Compiler

![Build](https://github.com/Archie3d/aida/actions/workflows/build-and-test.yml/badge.svg)

AIDA is an experimental Ada compiler written in C++ that translates a substantial
Ada subset into native programs. It uses [QBE](https://c9x.me/compile/) for code
generation and includes a C runtime and predefined Ada packages. It is not a
full Ada implementation.

## Build

Requirements: CMake 3.20 or newer, a C++23 compiler, a C11 compiler, and `make`
(used to build QBE).

macOS and Linux:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Windows: use an MSYS2 MinGW-w64 shell with GCC, make, and CMake installed.

The compiler produces programs for the build host/toolchain; cross-compilation
is not currently exposed as a supported driver option. To install, run
`cmake --install build --prefix <prefix>`, then put `<prefix>/bin` on `PATH`.

## Use example

```ada
-- hello.adb
with Ada.Text_IO;

procedure Hello is
begin
   Ada.Text_IO.Put_Line ("Hello, Ada!");
end Hello;
```

Compile and run it with the installed driver:

```sh
ada -o hello hello.adb
./hello
```

`ada --help` lists driver options. The driver resolves dependencies, invokes
the frontend, QBE, and the system C compiler, then links the runtime. For
frontend-only use, `adac --help` describes its options. Executable builds keep
per-unit artifacts in `<program>.adaobj` by default and reuse unchanged units.

Pass external C objects and static libraries directly when building an executable:

```sh
cc -c helper.c -o helper.o
ada -o program main.adb helper.o libexample.a
```

Declare C entry points with `pragma Import (C, Ada_Name, "c_symbol")`.
External `.o`/`.a` inputs retain their order after the Ada objects and before the
runtime; put archives after objects or archives that use them. The last Ada
source selects the main program and default output name. At least one Ada source
is required; `.o`/`.a` inputs cannot be used with `-S` or `--emit-ir`.
Use `--source file.a` for Ada sources with an archive extension (such as ACATS).
For manual linking, `ada -S -o program.s main.adb` produces whole-program
assembly; assemble it with `cc -c` and link with the AIDA runtime and its system
dependencies (`ada -v` shows the toolchain's link command).

## Architecture

The repository consists of the following components:
- `ada/`: driver, dependency scanning, binding, toolchain invocation, and cache.
- `adac/`: C++ frontend: lexer, parser, semantic analysis, and QBE IR emitter.
- `qbe/`: bundled QBE backend, translating its IR to native assembly.
- `runtime/`: C runtime plus predefined Ada packages (in `runtime/ada/`).
- `common/`: shared compiler utilities;
- `tests/`: compiler and runtime regressions.

The compilation path is Ada source -> AIDA frontend -> QBE IR -> QBE assembly -> system assembler/linker, with the AIDA runtime linked into the executable.
The [lowering decision](docs/lowering-decision.md) retains direct QBE emission
and records the representation and cleanup refactors required by later phases.

The [ACATS backfill baseline](docs/acats-baseline.md) records independent passes, failures and harness gaps for completed features.

## Features and limits

AIDA targets Ada 2005 and currently implements a substantial subset. Full
conformance and Specialized Needs Annex support are not claimed.

Supported:
- Library and nested packages, procedures, functions
- Extended returns, limited records and build-in-place results ([limits](docs/returns.md))
- Package elaboration blocks with local storage, captures and scoped cleanup
- Control flow, exceptions and names qualified by enclosing declarations ([contract](docs/enclosing-names.md))
- Case selectors resolve overloaded calls using the discrete result type ([rules](docs/case-selectors.md))
- Integer, floating-point, fixed-point, modular, array and record types
- Ordinary fixed point with wider binary scales, Small clauses and dynamic local bounds ([limits](docs/fixed-point.md))
- Decimal fixed point with 1–18 digits, exact scaling, Round and Decimal_IO ([limits](docs/decimal-fixed-point.md))
- Modular types through `2 ** 64`, modular generics, `'Mod` and `Modular_IO` ([limits](docs/modular-types.md))
- Exact bounded named-number arithmetic ([limits](docs/static-arithmetic.md))
- Checked binary32/64 arithmetic, integer powers and floating `'Value` ([floating model](docs/floating-point.md))
- Arrays with 64-bit bounds, lengths and byte strides, anonymous object declarations,
  library runtime bounds and generic objects; indices must be all constrained
  or all boxed (`range <>`) ([descriptor ABI](docs/array-descriptors.md))
- Runtime-sized array components and allocators, Size and array streams ([limits](docs/runtime-composites.md))
- One-dimensional concatenation with scalar, composite, private and controlled components
- Checked record and array aggregates, sliding, inferred bounds and `<>` defaults ([limits](docs/aggregate-defaults.md))
- Nested variants, default and runtime discriminants and per-object array components ([limits](docs/runtime-discriminants.md))
- Array conversions and writable parameter views (identical component subtypes required)
- Derived-type conversions and inherited profiles with checked constraints
- Scalar `'Value` uses the base range; receiving contexts check subtype constraints
- Derived Boolean operations and byte-character literals and strings
- Scalar attributes and captured runtime bounds, including library and real subtypes ([contract](docs/runtime-scalars.md))
- Generics with private/indefinite, discriminated, derived, tagged, interface and access formals ([limits](docs/generic-types.md));
  abstract formals, null defaults, attribute/literal actuals, nonstatic array matching,
  formal packages and class-wide derived actuals with dispatch wrappers;
  tagged dispatching and prefixed calls ([limits](docs/prefixed-calls.md))
- Named and anonymous object access, access-to-constant, accessibility checks,
  nested/protected callbacks and coextensions ([limits](docs/anonymous-access.md));
  object [`Unchecked_Access`](docs/unchecked-access.md)
- Imported Ada/C and inherited tagged callbacks ([limits](docs/subprogram-access.md))
- Null exclusions with checked initialization, calls and conversions
  ([limits](docs/null-exclusions.md))
- Primitive inheritance, overriding checks, operator visibility and dispatch through private overrides ([operators](docs/operator-visibility.md))
- Abstract subprograms, required overrides and dispatching calls ([limits](docs/abstract-subprograms.md))
- Overriding indicators, null procedures and controlling access results
  ([contract](docs/overriding-null-procedures.md))
- Controlled types, checked assignment and finalization ([contract](docs/controlled-assignment.md))
- Entry-free tasks, grouped activation, master-based waiting and task identity/status
- Protected types and single objects with procedures, functions and guarded entries
- `Duration`, relative/absolute delays and monotonic `Ada.Real_Time`
- Scalar `Atomic` and `Volatile` objects and types
- Size, alignment, enumeration and byte-addressable record clauses ([limits](docs/representation-clauses.md))
- Text and stream I/O
- Limited/private context clauses and incomplete views ([limits](docs/context-views.md))
- Separate compilation and incremental builds, including subprogram/package
  subunits ([limits](docs/subunits.md))
- Elaboration checks and `Elaborate`, `Elaborate_All`, `Elaborate_Body` pragmas

Not supported yet:
- Task entries, entry families, select/requeue, abort, and task/protected discriminants
- Aggregates, function results, tagged and variant records containing task or
  protected objects
- Controlled/task/protected elements in discriminant-dependent arrays
- Runtime-sized tagged records, tagged dependent components and delayed private-component layouts
- Concurrent interface implementations, task/protected subunits and formal packages
- Writable untagged scalar/record conversions and new derived discriminant parts
- Derivation before a private view is completed
- Wide character types and their string literals
- Named protected callback types and general C callback exports
- Many representation clauses and pragmas
- Standard libraries such as `Ada.Strings.Unbounded`, `Ada.Containers` and
  `Ada.Calendar`

Access-type collections support allocation and deallocation across tasks, with
master-based lifetime and finalization ([contract](docs/shared-collections.md)).

[Static arithmetic](docs/static-arithmetic.md) uses checked exact evaluation,
bounded to 127-bit integer/rational magnitudes and 64-bit scaled fixed-point
intermediates. Exceeding these limits is diagnosed; arbitrary precision remains
unsupported.

See [TODO.md](TODO.md) for the roadmap and `tests/` for executable examples
and expected diagnostics. The [Ada 2005 coverage matrix](docs/ada2005-coverage.md)
tracks known gaps; the [selected ACATS baseline](tests/acats/README.md) runs with
`ctest --test-dir build -L acats --output-on-failure`, including recorded expected
failures. This selection does not establish conformance.
