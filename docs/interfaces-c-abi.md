# Interfaces.C target ABI contract

Phase 13 targets the Ada 2005 packages `Interfaces`, `Interfaces.C`,
`Interfaces.C.Strings` and `Interfaces.C.Pointers`. The existing
[C Import and callback subset](subprogram-access.md) provides the starting
point. The packages and the compiler changes below are planned work.
[Ada 2005 RM B.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-B-2.html),
[B.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-B-3.html),
[B.3.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-B-3-1.html),
[B.3.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-B-3-2.html).

## Target C data layouts

Each `size/align` entry gives `sizeof` and `_Alignof` in C bytes. All five
targets use 8-bit bytes and little-endian storage. `SCHAR_MIN`, `SCHAR_MAX` and
`UCHAR_MAX` are −128, 127 and 255 on each.

| C type | Size/align | Signedness or format |
| --- | --- | --- |
| `char` | 1/1 | Varies by target |
| `signed char`, `unsigned char` | 1/1 | Signed, unsigned respectively |
| `short`, `unsigned short` | 2/2 | Signed, unsigned respectively |
| `int`, `unsigned int` | 4/4 | Signed, unsigned respectively |
| `long long`, `unsigned long long` | 8/8 | Signed, unsigned respectively |
| `size_t` | 8/8 | Unsigned |
| `ptrdiff_t` | 8/8 | Signed |
| `char16_t`, `char32_t` | 2/2, 4/4 | Unsigned |
| `void *` | 8/8 | Native pointer |
| `float` | 4/4 | IEEE binary32 |
| `double` | 8/8 | IEEE binary64 |

| Target | C data model | `long` and `unsigned long` size/align | Plain `char` | `wchar_t` size/align, signedness | `long double` size/align, format | Probe evidence |
| --- | --- | --- | --- | --- | --- | --- |
| Darwin arm64 | LP64 | 8/8 | Signed | 4/4, signed | 8/8, binary64 | Native Apple Clang and target compile |
| Darwin amd64 | LP64 | 8/8 | Signed | 4/4, signed | 16/16, x87 extended | Target compile only |
| Linux amd64 | LP64 | 8/8 | Signed | 4/4, signed | 16/16, x87 extended | Target compile only |
| Linux arm64 | LP64 | 8/8 | Unsigned | 4/4, unsigned | 16/16, binary128 | Target compile only |
| Windows amd64, GNU ABI | LLP64 | 4/4 | Signed | 2/2, unsigned | 16/16, x87 extended | Target compile only |

`long` is signed and `unsigned long` is unsigned on all five targets. Their
sizes follow the C data model independently of Ada `Long_Integer`. `plain_char`
selects the signed or unsigned character subtype according to the plain C `char`
column. C integer signedness governs layout and call handling independently of
Ada character and enumeration semantics.

The floating formats have radix 2. The table gives the C `<float.h>` values.

| Format | `MANT_DIG` | `MIN_EXP` | `MAX_EXP` |
| --- | ---: | ---: | ---: |
| binary32 | 24 | −125 | 128 |
| binary64 | 53 | −1021 | 1024 |
| x87 extended | 64 | −16381 | 16384 |
| binary128 | 113 | −16381 | 16384 |

The x87 format has an 80-bit encoding in a 16-byte C object on these targets.
The Windows row uses the GNU/MinGW target selected by the project’s
MSYS2 UCRT64 configuration. C compiler options such as `-funsigned-char`,
`-fshort-wchar` or `-mlong-double-64` can change individual facts. Future
package selection must use the supported target compiler defaults and keep the
frontend, runtime, QBE target and linker toolchain on the same ABI. See the
[native host matrix](host-validation.md#native-matrix).

## Probe and evidence

The existing `host.abi_layout` test runs `host_abi_report`, built with the
CMake C compiler. Its JSON preserves the earlier layout fields and adds the
C scalar data, including sizes, alignments, integer signedness, floating
formats and precision/exponent parameters. The native build and profiling
workflows publish this report in `abi.json`. A `format` value of `unknown`
keeps the measured parameters available for review.

```sh
cmake --build build --target host_abi_report
ctest --test-dir build -R '^host\.abi_layout$' --verbose
```

The table was checked with Apple Clang 21.0.0. Darwin arm64 was executed
natively. Clang also generated objects and LLVM IR using each of the five
explicit target triples. This measures C compiler layouts at compile time
without a target sysroot or target execution. The `char16_t` and `char32_t`
entries use their C11 underlying typedefs, `uint_least16_t` and `uint_least32_t`.
The following command shows the Windows GNU facts.

```sh
clang --target=x86_64-w64-windows-gnu -std=c11 -ffreestanding \
    -S -emit-llvm tests/host/CAbiFacts.c -o - | rg '^@kCAbi(Target|Integers|Floats) ='
```

Use `arm64-apple-darwin`, `x86_64-apple-darwin`, `x86_64-unknown-linux-gnu`,
`aarch64-unknown-linux-gnu` and `x86_64-w64-windows-gnu` to reproduce the matrix.
Linux, MSYS2 UCRT64 GCC and Darwin amd64 still need native runs of the expanded
probe and future Ada/C call tests. The target compiles do not add a cross-build
mode to `ada`.

## Native long double policy

Use QBE’s existing `d` scalar for `Interfaces.C.long_double` only when the
target C compiler provides binary64 with the same size, alignment, and
argument and result convention as C `double`. Keep the Ada type distinct
from `Interfaces.C.double`. Darwin arm64 is the candidate in this matrix.
An exploratory native probe called a C `long double` function directly and
through a C-convention access value using Ada `Long_Float`. Ten floating-point
arguments exercised registers and the stack, and the returned value agreed in
both whole and per-unit builds. Repeat these checks with the package type before
claiming package support.

For x87 extended, binary128 or an unrecognized format, keep the supported
declarations in `Interfaces.C` usable and diagnose use of `long_double` with
the target and format before lowering. Apply the same gate to any corresponding
hardware floating type exposed through `Interfaces`. A C wrapper can keep
extended-precision data and arithmetic on the C side, with Ada passing an opaque
address through a supported profile. General Ada operations or by-value
interop for those formats remain deferred. [QBE IL scalar types](../qbe/doc/il.txt).

## Completion criteria

| Scope | Required evidence |
| --- | --- |
| `Interfaces` (B.2) | Pure package with the signed and modular 8/16/32/64-bit types, intrinsic shifts and rotates, and documented floating formats. Check exact storage, shift/rotate boundary counts, distinct identities, generic matching and the unsupported-format diagnostics. |
| `Interfaces.C` (B.3) | Pure package with target-compatible scalars, constants and character arrays. Check C size/alignment/stride and high-bit values, full-width `size_t` indices and checked allocation extents. Exercise both forms of the conversion APIs with nonzero bounds, null arrays, embedded or missing NULs and mapping failures. Prove native scalar, pointer and array calls against C helpers, including access-to-subprogram calls, C element pointers without Ada descriptor arguments, and diagnostics for unsupported profiles. |
| `Interfaces.C.Strings` (B.3.1) | Preelaborated package with a native `chars_ptr` and the specified null/default initialization, borrowed views, owned allocation, release, length, value and update operations. Exercise required exceptions, allocation failure, cleanup and concurrency, and document allocator and lifetime obligations. |
| `Interfaces.C.Pointers` (B.3.2) | Preelaborated generic package preserving element identity and stride, supporting pointer arithmetic, difference, terminated and counted values and copies. Check null and length cases, nonzero bounds and generic matching. Document caller obligations for raw native pointers. |
| Supported targets and integration | Record a native C report and Ada/C calls with the supported C toolchain on every target claimed. Run whole/per-unit and deterministic IR checks, installed package discovery, the full suite, applicable sanitizer runs and independently graded selected ACATS tests. Update the predefined inventory and coverage matrix, recording each remaining target or API gap. |

General `Export`, Ada-defined C callbacks, foreign objects, by-value records,
variadics and C++ bindings remain separate roadmap follow-ups.
