# AIDA Ada Compiler

![Build](https://github.com/Archie3d/aida/actions/workflows/build-and-test.yml/badge.svg)

AIDA is an experimental native compiler for a substantial subset of Ada 2005.
It uses a C++ frontend, the [QBE backend](https://c9x.me/compile/), a C runtime
and predefined Ada packages. See the [coverage matrix](docs/ada2005-coverage.md)
for supported behavior and known gaps.

## Build and install

Build on macOS arm64, Linux amd64 or arm64, or Windows amd64 with MSYS2 UCRT64.
You need CMake 3.20 or newer, a C++23 compiler, a C11 compiler and `make` for
QBE. On Windows use the UCRT64 shell with MinGW-w64 GCC and CMake.

```sh
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix /path/to/install
```

Add the installation's `bin` directory to `PATH`. The driver targets its build
host and toolchain. On MSYS2, add `-G 'MinGW Makefiles'` to the configure
command. [Native test matrix and ABI coverage](docs/host-validation.md).

## Compile and run

Save this as `hello.adb`.

```ada
with Ada.Text_IO;

procedure Hello is
begin
    Ada.Text_IO.Put_Line ("Hello, Ada!");
end Hello;
```

```sh
ada -o hello hello.adb
./hello
```

`ada --help` lists driver options. Use `adac --help` for frontend-only options.
The driver resolves Ada dependencies, binds elaboration order, invokes the
frontend and QBE, and links with the system C toolchain. Executable builds keep
per-unit artifacts in `<program>.adaobj` by default. See the
[driver documentation](docs/driver-builds.md) for incremental builds and cleanup.

Link C objects and static libraries by passing them to the driver. Declare their
entry points with `pragma Import`. See the [C ABI contract](docs/interfaces-c-abi.md)
for supported profiles and target limits.

```sh
cc -c helper.c -o helper.o
ada -o program main.adb helper.o
```

## Documentation

- [Documentation index](docs/README.md) for compiler usage, language and runtime
  contracts, design notes and validation.
- [Roadmap](TODO.md) for priorities and completed phases.
- [Test fixtures](tests/ada/README.md) for runnable examples and expected
  diagnostics.
- [ACATS guide](tests/acats/README.md) for running the selected suite. The results
  apply to the documented subset.
