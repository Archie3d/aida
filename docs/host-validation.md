# Host and target validation

Phase 8 has native **Darwin arm64** evidence and prepared checks for the other
hosts. The roadmap item remains open until their native runs and matched
performance comparisons are available. CI configuration is not execution evidence.

## Native evidence

The local run used Darwin 27 arm64, Apple Clang 21.0.0, a Release build and
`lib64`. The CMake source path (a symlink to the checkout) and build path contained spaces. The full
suite passed **1,937/1,937**, including all five `host` checks; seven distinct
focused ASan/UBSan checks passed. A separate install check also passed with the
default `lib` layout. All 145 raw ACATS baseline observations are unchanged.
[Machine-readable evidence and timing samples](host-validation-darwin-arm64.json).

The ABI probe checks C/Ada integer and floating widths, signed extremes,
unsigned 64-bit values, addresses, mixed register/stack arguments and writes
through Ada object addresses. It runs whole/per-unit compilation and deterministic
IR checks. The native layout report verifies the compiler/runtime's 64-bit,
little-endian model, trace frames and array descriptors against actual C layouts.
C `long` is reported separately: LP64 on this host; Windows LLP64 must report
32 bits without changing Ada Long_Integer's 64-bit ABI.

Installation validation installs into a temporary prefix, moves it, and compiles
from an unrelated directory. Source, dependency, executable and install names
contain spaces. Driver/frontend invocation by absolute path and PATH, POSIX
symlinks, ADA_INCLUDE_PATH, ADA_LIBRARY and ADA_RUNTIME are checked. A sentinel
in the installed System source makes fallback to build-tree libraries fail;
verbose commands verify the relocated frontend, QBE and runtime archive are used.
The build/source tree is not renamed or removed. Absolute install destinations
are rejected by the test before installation; the tested layouts use relative
`bin` and `lib`/`lib64` destinations. The host C toolchain remains required.

The shared path parser fixes Windows drive-letter splitting in PATH,
ADA_INCLUDE_PATH and ADA_RUNTIME, preserves spaces, recognizes both directory
separators and supports `.exe` discovery for bare PATH invocations. Lexical
Windows controls run locally; native Windows process/path behavior is pending.

## Prepared native matrix

macOS validation is performed locally rather than in GitHub Actions. Darwin
arm64 has the native evidence above; Darwin amd64 remains unvalidated and
requires a separate manual run if that target is needed.

The [workflow](../.github/workflows/build-and-test.yml) runs Linux and MSYS2 jobs,
checks out into a spaced source path, builds Release into a spaced build path and runs the complete
suite on each host. No host-specific test exclusion or expected-failure blanket
was introduced. Installation and ABI checks belong to the normal CTest suite.

| Host | Prepared runner | Execution status |
| --- | --- | --- |
| Linux amd64 | ubuntu-24.04, lib64 | Pending native run |
| Linux arm64 | ubuntu-24.04-arm, lib | Pending native run |
| Darwin amd64 | Manual validation | Unvalidated; no CI job |
| Darwin arm64 | Local development, lib/lib64 | Local native pass; no CI job |
| Windows amd64 | windows-2025, MSYS2 UCRT64/GCC, lib | Pending native run, including threading |

Runner labels follow GitHub's [native runner inventory](https://docs.github.com/en/actions/reference/runners/github-hosted-runners).
Windows uses the documented [MSYS2 UCRT64 setup](https://github.com/msys2/setup-msys2).
The workflow preserves JUnit, CTest diagnostics, relocation logs, ABI facts and
current timing samples per runner. It has not been dispatched from this session.
Cross-assembly controls elsewhere in the suite are not native execution evidence.
The driver continues to target its build host/toolchain; no cross-compilation
option, 32-bit target or big-endian target is added.

## Performance evidence

`tests/host/measure.py` adds repeatable, alternating comparisons for scalar
calls, image results, block-heavy scopes and allocation/deallocation. Both
binaries validate their results; compilation is outside the timed interval.
Process wall time includes startup. The context-entry probe records its own
loop time for modes 0/1/2 in alternating order. There is no CTest timing threshold.

Eleven local runs after build/test work finished gave these current-only medians:

| Workload | Seconds |
| --- | ---: |
| Two million generated scalar calls | 0.013617 |
| Integer/fixed image results | 0.023794 |
| 200,000 block scopes | 0.023133 |
| 100,000 allocation/deallocation pairs | 0.025576 |
| Separate accessor/entry model | 0.028954 |
| Fused traceback entry model | 0.028616 |
| Hidden-parameter entry model | 0.029676 |

These are current compiler/runtime measurements, not an actual pre-Phase-2
comparison or proof of the no-slowdown goal. A three-run identical-driver paired
control exercised the comparison harness and is explicitly labeled as such.
CI likewise records current-only samples until a matching baseline toolchain
is supplied. The existing [historical locking costs and performance limits](../tests/benchmarks/README.md)
remain applicable; they are not replaced with a no-regression claim.

## Reproduce

```sh
cmake -S . -B 'build with spaces' -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_LIBDIR=lib64
cmake --build 'build with spaces' --parallel 3
ctest --test-dir 'build with spaces' --output-on-failure --parallel 3
ctest --test-dir 'build with spaces' -L host --output-on-failure
cmake --build 'build with spaces' --target context_access_benchmark
python3 -B tests/host/measure.py --ada 'build with spaces/ada/ada' \
    --build 'build with spaces' \
    --context-probe 'build with spaces/tests/context_access_benchmark' \
    --work 'build with spaces/host evidence' --runs 11
```

On MSYS2 use UCRT64, `-G 'MinGW Makefiles'`, native Python and `.exe` tool names.
For a real before/after comparison, add `--baseline-ada /path/to/before/ada`
and `--baseline-build /path/to/before/build`; each driver must retain its matching
frontend/QBE/runtime. Run on the same host and toolchain without concurrent
validation load. Review the samples and workload coverage before making a
performance conclusion.
