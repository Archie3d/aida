# Host and target validation

Phase 8 has successful native **Darwin arm64, Linux amd64/arm64 and Windows
amd64 (MSYS2 UCRT64)** evidence. The corrected CI runs pass 1,938/1,938 tests
on each Ubuntu architecture and 1,937/1,937 on Windows, with no failures,
disabled tests or skips. All eight [MSYS2 fixes](msys2-ci-fixes.md) are confirmed.
Matched comparisons now cover all active CI hosts. The roadmap item remains
open for the block/allocation regressions described below.
[Pinned current and previous CI results](host-validation-ci.json).

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
Windows controls run locally; native MSYS2 source-alias, PATH and relocation
checks also pass.

## Native matrix

macOS validation is performed locally rather than in GitHub Actions. Darwin
arm64 has the native evidence above; Darwin amd64 remains unvalidated and
requires a separate manual run if that target is needed.

The [workflow](../.github/workflows/build-and-test.yml) runs Linux and MSYS2 jobs,
checks out into a spaced source path, builds Release into a spaced build path and runs the complete
suite on each host. No host-specific test exclusion or expected-failure blanket
was introduced. Installation and ABI checks belong to the normal CTest suite.

| Host | Prepared runner | Execution status |
| --- | --- | --- |
| Linux amd64 | ubuntu-24.04, lib64 | Native CI: 1,938/1,938 PASS |
| Linux arm64 | ubuntu-24.04-arm, lib | Native CI: 1,938/1,938 PASS |
| Darwin amd64 | Manual validation | Unvalidated; no CI job |
| Darwin arm64 | Local development, lib/lib64 | Local native pass; no CI job |
| Windows amd64 | windows-2025, MSYS2 UCRT64/GCC, lib | Native CI: 1,937/1,937 PASS; all eight fixes confirmed |

Runner labels follow GitHub's [native runner inventory](https://docs.github.com/en/actions/reference/runners/github-hosted-runners).
Windows uses the documented [MSYS2 UCRT64 setup](https://github.com/msys2/setup-msys2).
The workflow preserves JUnit, CTest diagnostics, relocation logs, ABI facts and
current timing samples per runner. The user supplied successful native
Linux/MSYS2 rerun artifacts. Linux has one more test overall: its POSIX host and
terminal checks are replaced by the native Windows host-library check on MSYS2.
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
The initial CI records were current-only; the subsequent native runs now
include historical pairs on all three active hosts. The existing [historical locking costs and performance limits](../tests/benchmarks/README.md)
remain applicable; they are not replaced with a no-regression claim.

### Matched historical comparison

Native Darwin arm64 measurements now compare the pre-Phase-2 revision
`3d2aac5980f57fb68524a15b154f65cf0882bdc2`, the end-of-Phase-2 revision
`ac72cd62f6fe75a03d1e97b61e61b3f516a85d30`, and current compiler revision
`cf5f8706b4106695fb0561ca4f3af97cecb8f058`. Each compiler uses its own matching
runtime and library sources. All builds use Release, the same Apple toolchain,
and QBE revision `e786f06032fefa2e3790d6b1c9e31ed138f475a6`. Each pair validates
both programs, warms them once and alternates execution order for 21 trials;
builds finish before timing starts.

| Workload | Pre-Phase-2 median (s), current pair | Current median (s) | Current / pre | End Phase 2 / pre, separate pair |
| --- | ---: | ---: | ---: | ---: |
| Scalar calls | 0.018836 | 0.014231 | 0.756 | 0.894 |
| Images | 0.030324 | 0.023696 | 0.781 | 0.867 |
| Block scopes | 0.004661 | 0.022396 | 4.805 | 1.036 |
| Allocation/deallocation | 0.006477 | 0.024913 | 3.847 | 1.049 |

[Full samples, component hashes, revisions and build flags](host-performance-darwin-arm64.json)
are retained in the repository, independently of downloaded CI artifacts.
The large block/allocation regressions appear after Phase 2; this comparison
does not isolate an individual later commit. Generated code brackets these
scopes with master hooks. Unlike Phase 2's empty hooks, current master await/leave
performs shared-table locking and lookup, including a second await inside leave;
allocation also uses access-collection bookkeeping. These are investigation
targets, not a measured attribution of all overhead. The small Phase-2-only
differences need longer workloads before interpreting them as regressions.
No broad no-slowdown claim is supported.

The CI workflow now checks out the pinned pre-Phase-2 revision with its QBE
submodule and builds its `ada` target separately in Release. The existing native
suites still run against current code; timing collection then alternates current
and historical binaries for 21 trials. The supplied Linux/MSYS2 artifacts confirm successful historical builds and
workload validation. macOS CI entries remain commented out.

### Native CI historical comparisons

The next supplied CI run passes 1,938/1,938 tests on both Ubuntu architectures
and 1,937/1,937 on MSYS2, with no failures, disabled tests or skips. All three
relocated installs and ABI controls pass. Each host supplies 21 samples per
workload and compiler, using matching Release flags and compiler versions.
Workload hashes match the repository sources (CRLF checkout on Windows).

| Host | Scalar calls, current / baseline | Images | Blocks | Allocation |
| --- | ---: | ---: | ---: | ---: |
| Ubuntu amd64 | 0.707 | 0.804 | 9.800 | 8.044 |
| Ubuntu arm64 | 0.919 | 0.930 | 8.811 | 8.091 |
| MSYS2 UCRT64 amd64 | 0.729 | 1.060 | 5.451 | 3.901 |

Ratios above one are slower. The block/allocation regression is present on
all measured hosts. Windows images are about 6% slower in this run; process
startup and runner variability require longer measurements before attributing
that smaller difference. Each ratio compares the two compiler builds on one host; it does not compare
host speeds. The baseline medians are only a few milliseconds
for blocks/allocation, but their large differences also reproduce locally.

[Full unmodified timing reports](host-performance-ci.json) retain every sample,
revision field, compiler/flags and component hash. [JUnit/install summaries](host-validation-ci.json)
retain current and previous artifact checksums. Linux reports current revision
`c4fdd097f06e208ec9ef33c81177230f4c675925`; all three report the pinned
pre-Phase-2 baseline. The Windows current-revision field is empty, so its exact
source revision cannot be established from that field; its measured binary
hashes remain available. The workflow now supplies `GITHUB_SHA` directly rather
than relying on a shell Git lookup, and the collector rejects explicitly empty
revision arguments. No source revision has been substituted into the old report.
The cross-host measurements are collected; the no-slowdown goal remains unmet.

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
