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

## Features and limits

The implementation targets Ada 95 with selected Ada 2005 features. Supported
areas include library and nested packages, procedures and functions, control
flow and exceptions; integer, floating-point, fixed-point, modular, array and
record types; generics; access types; tagged types and dispatch; controlled
finalization; and text and stream I/O. Separate compilation and incremental
builds are supported. The regression suite is the practical reference for
implemented behavior. Calls before body elaboration raise `Program_Error`,
including callbacks and renaming completions; failed elaboration runs cleanup.
Generic instances retain their specification bindings while awaiting a later
template body and check body elaboration before initializing the instance.

It does not yet implement the full Ada standard. In particular, tasking and
protected types, interfaces, `goto`, `separate` subunits, several access-type
features, formal packages and many representation clauses and pragmas are
unsupported. Newer standard features and libraries such as
`Ada.Strings.Unbounded`, `Ada.Containers`, and `Ada.Calendar` are also absent.
Unsupported constructs are intended to produce diagnostics rather than be
silently ignored. See [TODO.md](TODO.md) for the development roadmap and
`tests/` for executable examples and expected diagnostics.
