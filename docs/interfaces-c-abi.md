# Interfaces.C target ABI contract

Phase 13 targets the Ada 2005 packages `Interfaces`, `Interfaces.C`,
`Interfaces.C.Strings` and `Interfaces.C.Pointers`. The existing
[C Import and callback subset](subprogram-access.md) provides the starting
point. `Interfaces`, the scalar and character-array APIs of `Interfaces.C`,
and `Interfaces.C.Strings` are available.
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
`-fshort-wchar` or `-mlong-double-64` can change individual facts. The frontend
uses facts compiled by the configured C compiler. Foreign objects, runtime
and linker toolchain must use the same ABI. See the
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
    -S -emit-llvm common/CAbiFacts.c -o - | rg '^@kCAbi(Target|Integers|Floats) ='
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
from `Interfaces.C.double`. Darwin arm64 supports this representation. The
native test calls a C `long double` function directly and through a
C-convention access value using the package type. Ten floating-point
arguments exercise registers and the stack in whole and per-unit builds.

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

## Representation verification

`host.c_scalar_widths` calls C with both signed and unsigned 8-, 16-, 32- and
64-bit values. The test covers high-bit results, indirect calls, stack
arguments, modular conversions and range failures. C also updates arrays
through element addresses to verify four-byte unsigned word storage and
1-, 2- and 4-byte aliased character strides, including visibility through
an existing Ada access value.

`unsignedarraybounds` exercises static and dynamic ranges, returned and
allocated arrays, indexing, slices, aggregates, multidimensional views,
array conversions and checked count and byte-size overflow. The stream tests
verify that bounds cross the stream as unsigned Ada values. The tests cover
whole and per-unit builds and deterministic IR where applicable.

Both control programs run natively on Darwin arm64. The scalar fixture's
generated Ada assembly and C helper also compile to objects for the five
target ABIs in the matrix. Native execution on the other targets is part of
the phase's integration checks.

The representation increment's Darwin arm64 Release campaign covered 2,213 checks. Three container
compiles reached the harness deadlines during the parallel run and passed
on isolated reruns. Thirteen focused ASan/UBSan checks passed.

## Scalar packages

`Interfaces` is Pure and declares `Integer_8` through `Integer_64`,
`Unsigned_8` through `Unsigned_64`, and the distinct `IEEE_Float_32` and
`IEEE_Float_64` types. The five shift and rotate operations are Intrinsic for
each modular width. Shift counts at or above the width produce zero or the
arithmetic sign fill. Rotation counts wrap modulo the width. A negative count
fails the `Natural` parameter check, and intrinsic subprograms reject `'Access`.

`Interfaces.C` is Pure and declares the standard integer, modular and floating
types, the C character limit constants, and the four character types and NUL
constants. `long`, `unsigned_long`, `plain_char` and `wchar_t` follow the
configured C compiler. Character operations preserve Ada positions. Imported
C calls use the native ABI class, including the signedness of plain `char`.
`wchar_t` derives from `Wide_Character` at 16 bits and `Wide_Wide_Character`
at 32 bits. `char32_t` and 32-bit `wchar_t` admit the Ada `Wide_Wide_Character`
range. Foreign character results outside the Ada type's range raise
`Constraint_Error`.

The regression tests cover declarations and predicates against a C helper,
type identity, all four overloaded intrinsic profiles through generic formal
subprograms, a Pure client, whole and per-unit builds, deterministic IR,
source-free replay and a relocated installation. `host.interfaces_long_double`
checks the native call path or verifies target- and format-specific diagnostics
on unsupported targets.

Darwin arm64 passed all 2,217 Release checks across the full run and reruns
of the renamed tagged-interface fixture. The final focused Release run passed
16 checks, and 10 focused ASan/UBSan checks passed. The C facts, shift runtime
and Ada/C scalar fixture cross-assembled for all five targets. Simulated LLP64
and Linux arm64 facts exercised the alternate scalar layouts and the x87 and
binary128 diagnostics, including through limited views. Native execution on
the other four targets remains an integration check.

## Character arrays and conversions

`char_array`, `wchar_array`, `char16_array` and `char32_array` are unconstrained
arrays indexed by `size_t`, with aliased components. Their components have the
width of the corresponding C character type. The conversion pairs are `String`
with `char_array`, `Wide_String` with either `wchar_array` or `char16_array`, and
`Wide_Wide_String` with `char32_array`. Scalar conversions preserve the ordinal
value of each code unit. A 32-bit `wchar_t` outside the range of `Wide_Character`
raises `Constraint_Error` when converted to Ada.

`To_C` copies all input code units and appends a NUL by default. `To_Ada` trims
at the first NUL by default and raises `Terminator_Error` if it finds none.
`Trim_Nul => False` copies the full array, including any NULs.
`Is_Nul_Terminated` tests for a NUL anywhere in the array. The function results
start at zero for `To_C` and one for `To_Ada`. An empty input with
`Append_Nul => False` raises `Constraint_Error` in the function because a null
`size_t` range cannot start at zero. The procedure accepts that input and
returns a zero count. [Ada 2005 RM B.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-B-3.html).

The buffer procedures write starting at `Target'First` and raise
`Constraint_Error` if the result exceeds `Target'Length`. They set `Count` on
success to the number of code units written, including an appended terminator.
`To_Ada` checks for the required NUL before writing and skips any code units
after it when trimming. The functions use the compiler's caller-owned array
results and exception cleanup. Tests exercise both forms in whole and per-unit
builds, including bounds at `size_t'Last`, aliased element access, and errors.
The conversion fixture passed with native 32-bit `wchar_t` on Darwin arm64
and with simulated 16-bit `wchar_t` facts. All 2,220 Darwin arm64 Release checks
and six focused ASan/UBSan checks passed.

## Native C calls

Fixed-arity native `Import (C, ...)` declarations and named C callbacks share
one call convention. A compatible one-dimensional array contributes a single
pointer to its elements to the C argument list. This covers the four
`Interfaces.C` character arrays and rank-one arrays with naturally laid-out
integer, floating-point, compatible enumeration or native pointer elements.
Slice arguments start at the slice's first element. Constrained formals still
check the array shape. `out` and `in out` arrays allow C to write to their elements.

C code must observe the declared parameter mode, stay within the actual
buffer, and supply valid Ada component values when it writes. The caller must
pass an explicit length or arrange termination as required by the C routine.
An empty array has no element to dereference. C may borrow array actuals,
including temporary conversion results, for the duration of the call.
Retaining a pointer requires independently managed storage that outlives every
use of that pointer.

Native scalars use their C register and storage classes. Ordinary enumeration
values and results use word-sized C codes. Writable scalar and pointer formals
pass addresses of temporaries with the formal's storage width and checked
copy-back on normal return. Fixed-layout untagged records with naturally laid-out
scalar fields can also be passed by reference. Named
access-to-object values and anonymous object access parameters can designate
compatible scalar storage. Named C access-to-subprogram values and
`System.Address` also cross the boundary as native pointers. Ada's null
exclusions still apply. Memory reached through a returned pointer stays under
its original ownership. [Ada 2005 RM B.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-B-3.html).

`Convention (C, ...)` can check the layout of a named compatible array or
object access type and can select the native calling convention for a named
callback type. Unsupported profiles, including packed or multidimensional
array parameters, tagged record parameters, composite and anonymous access
results, pointers to composites, and Ada closure parameters, are diagnosed
before code generation.
Imports of reserved `__ada_` symbols use the compiler-runtime profiles.

`host.interfaces_native_calls` links a C helper in whole and per-unit builds
and verifies all four character layouts, mixed arguments, pointer results,
slice writes and indirect calls. The source-free interface fixture calls
`strlen` both directly and through a C callback after replaying the checked
package interface. The libc fixtures use `Interfaces.C.int` and
`Interfaces.C.size_t` for C integer parameters, results and byte counts.
The Ada fixture and C helper compile to objects for the five
target configurations in the matrix, using simulated target facts for
Windows LLP64 and Linux arm64. Runtime execution was verified on Darwin arm64.

The 2,224-test Release run found three regressions in the enumeration and
record layout fixtures. All three passed in the final 116-test Release sweep.
The 23 focused ABI checks and 13 libc fixture checks passed with ASan/UBSan.

## C strings

`Interfaces.C.Strings` is preelaborated. Its private `chars_ptr` holds a native
`char *` and defaults to `Null_Ptr`. A `chars_ptr_array` has aliased pointer
elements and can be passed directly to a C routine expecting `char **`.

`New_Char_Array` and `New_String` allocate a copy through the C allocator. The
copy ends at the first NUL or the end of the input and always includes a NUL
terminator. Allocation failure raises `Storage_Error`. Release the result
once with `Free`, which resets that variable to `Null_Ptr`. Any other copies
of the pointer become invalid as soon as the allocation is released.

`To_Chars_Ptr` borrows the bytes of an aliased Ada array. With `Nul_Check`
enabled it raises `Terminator_Error` for an array with no NUL. The caller must
keep the array alive for every use of the borrowed pointer. An Ada allocator
or a foreign library that owns a buffer must release it through its own
deallocator. `Interfaces.C.Strings.Free` is for the two allocation functions
in this package. [Ada 2005 RM B.3.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-B-3-1.html).

`Strlen` and `Value` without `Length` require a valid NUL-terminated buffer.
The counted forms read up to `Length` bytes and stop at the first NUL. The
caller must provide readable storage up to that bound or an earlier NUL.
An array result includes any NUL read and starts at zero. A string result
starts at one and omits the NUL. Null pointers raise `Dereference_Error`,
and a zero `Length` on a nonnull pointer raises `Constraint_Error`.

`Update` counts its byte offset from zero. With `Check => True` it raises
`Update_Error` for a write that would overwrite or skip past the current
terminator. Its copy supports overlap with the source array. An unchecked update
requires the caller to satisfy the same offset and length limits and provide a
writable buffer. An empty `String` follows `To_C` with `Append_Nul => False`
and propagates `Constraint_Error`.

The tests exercise borrowed and owned storage, independent concurrent calls,
exact-size C buffers, and allocation failure by substituting the allocator
only in the test translation unit. They also cover whole and per-unit builds,
source-free interface replay, and installation relocation.

On Darwin arm64, all 2,229 Release checks and 14 focused ASan/UBSan checks
passed. The four-worker test passed with a ThreadSanitizer-instrumented C
runtime. An ASan shadow-memory check confirmed that `Free` released the C
allocation.
