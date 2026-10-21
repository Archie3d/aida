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

## Features and limits

AIDA targets Ada 2005 and currently implements a substantial subset. Full
conformance and Specialized Needs Annex support are not claimed.

Supported:
- Library and nested packages, procedures, functions
- Package elaboration blocks with local storage, captures and scoped cleanup
- Control flow and exceptions
- Integer, floating-point, fixed-point, modular, array and record types
- Checked binary32/64 arithmetic and negative integer powers ([floating model](docs/floating-point.md))
- Checked record and array aggregates, including sliding and inferred bounds
- Nested variant records with checked selection and constrained assignment
- Array conversions and writable parameter views (identical component subtypes required)
- Derived-type conversions and inherited profiles with checked constraints
- Derived Boolean operations and byte-character literals and strings
- Scalar position, value and adjacent-value attributes; scalar and array bounds
- Generics, access types, tagged types with dispatching
- Primitive inheritance, overriding checks and dispatch through private overrides
- Controlled types and finalization
- Entry-free tasks, grouped activation, master-based waiting and task identity/status
- Protected types and single objects with procedures, functions and guarded entries
- `Duration`, relative/absolute delays and monotonic `Ada.Real_Time`
- Scalar `Atomic` and `Volatile` objects and types
- Text and stream I/O
- Separate compilation and incremental builds
- Elaboration checks and `Elaborate`, `Elaborate_All`, `Elaborate_Body` pragmas

Not supported yet:
- Task entries, entry families, select/requeue, abort, and task/protected discriminants
- Aggregates, function results, tagged and variant records containing task or
  protected objects
- Ada 2005 box defaults (`<>`) in aggregates
- Interfaces, `goto`, `separate` subunits, formal packages
- Writable untagged scalar/record conversions and new derived discriminant parts
- Derivation before a private view is completed
- String literals and concatenation with character components wider than a byte
- Several access-type features, many representation clauses and pragmas
- Standard libraries such as `Ada.Strings.Unbounded`, `Ada.Containers` and
  `Ada.Calendar`

Access-type collections remain owned by their creating task; allocation or
deallocation through another task raises `Program_Error`.

[Static arithmetic](docs/static-arithmetic.md) uses checked exact evaluation,
bounded to 127-bit integer/rational magnitudes and 64-bit scaled fixed-point
intermediates. Exceeding these limits is diagnosed; arbitrary precision remains
unsupported.

See [TODO.md](TODO.md) for the roadmap and `tests/` for executable examples
and expected diagnostics. The [Ada 2005 coverage matrix](docs/ada2005-coverage.md)
tracks known gaps; the [selected ACATS baseline](tests/acats/README.md) runs with
`ctest --test-dir build -L acats --output-on-failure`, including recorded expected
failures. This selection does not establish conformance.
