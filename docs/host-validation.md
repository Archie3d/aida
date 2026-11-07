# Host and target validation

Phase 8 has successful native **Darwin arm64, Linux amd64/arm64 and Windows
amd64 (MSYS2 UCRT64)** evidence. The latest CI runs pass 1,942/1,942 tests
on each Ubuntu architecture and 1,941/1,941 on Windows, with no failures,
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
| Linux amd64 | ubuntu-24.04, lib64 | Native CI: 1,942/1,942 PASS |
| Linux arm64 | ubuntu-24.04-arm, lib | Native CI: 1,942/1,942 PASS |
| Darwin amd64 | Manual validation | Unvalidated; no CI job |
| Darwin arm64 | Local development, lib/lib64 | Local native pass; no CI job |
| Windows amd64 | windows-2025, MSYS2 UCRT64/GCC, lib | Native CI: 1,941/1,941 PASS; all eight fixes confirmed |

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

### Empty-master cleanup repair

The next repair gives each master monotonic atomic activity flags. Task creation
marks its master and ancestor chain before publishing the task under the shared
mutex; reparenting a task-bearing collection marks its new ancestors as well.
Activation/await can skip the registry for masters without dependents, and leave
only locks when removing a registered master or accessibility record. Empty
blocks avoid five lock/unlock pairs. Dependent-task joining still resamples the
registry, and leave retains its defensive await after finalization.

The master record grows from 8 to 16 bytes. Compiler scratch allocations and
the C runtime share `common/Master.h`; the native ABI report checks the size and
activity-field offset. Captured constructor-master slots remain pointers.
Recompile Ada objects with the matching compiler/runtime.

Two 21-trial native Darwin arm64 Release pairs compare the working tree with
the preceding revision `5a08a8e66f046008bd8b942ed6eea099edae1951` and separately
with pre-Phase-2. Builds/tests finish before timing, and both builds in each
pair use matching flags and compiler versions.

| Workload | Before repair median (s) | After repair median (s) | After / before | After / pre-Phase-2, separate pair |
| --- | ---: | ---: | ---: | ---: |
| Scalar calls | 0.013650 | 0.013665 | 1.001 | 0.754 |
| Images | 0.023391 | 0.023413 | 1.001 | 0.782 |
| Block scopes | 0.022801 | 0.016716 | 0.733 | 3.475 |
| Allocation/deallocation | 0.025279 | 0.018615 | 0.736 | 2.795 |

[All samples, component/source hashes, flags and validation details](host-performance-master-activities.json)
are retained separately from the previous CI evidence. This removes a measured
contributor, but does not close the remaining regression. Inspection of generated
block IR shows `__ada_array_local` for the small aggregate, a copy to the fixed
local array, and arena rewind/free each iteration; the pre-Phase-2 IR does not
allocate that temporary. Allocation/deallocation still locks the shared storage
registry for lookup and membership changes. These are the next investigation
targets, without attributing all remaining time to either one.

The Release full run passed 1,937/1,938 checks; its only failure was the expected
master scratch allocation change in `ir.hello`. Reviewing all generated units
found exactly three `alloc8 8` to `alloc8 16` changes in two golden files. After
updating those references, all six focused reruns pass, so all 1,938 distinct
Release checks have passed. Eleven distinct ASan/UBSan and five TSan checks pass,
covering real task lifecycle, shared collections, accessibility, constructor
lifetimes and generated master-hook cleanup. The lifecycle regression holds the
shared mutex across empty-scope operations to detect unwanted locking and
checks expired access levels and reparented unactivated tasks deterministically.

Validation also corrected the lifecycle fixture to use a real runtime collection,
passed sanitizer link flags to the master-hook fixture, and made TSan adapter
checksums resolve relative to the source directory during CMake regeneration.
The subsequent native CI run confirms the new layout and records the following
pairs against pre-Phase-2. Both Ubuntu suites pass 1,938/1,938 checks; MSYS2
passes 1,937/1,937, with no failures, disabled tests or skips. Task lifecycle,
shared collections, accessibility, ABI and relocated-install checks pass on all
three. Every ABI report records `master_bytes: 16`.

| Host | Scalar calls, current / historical | Images | Blocks | Allocation |
| --- | ---: | ---: | ---: | ---: |
| Ubuntu amd64 | 0.680 | 0.827 | 5.817 | 5.308 |
| Ubuntu arm64 | 0.920 | 0.915 | 5.723 | 5.939 |
| MSYS2 UCRT64 amd64 | 0.746 | 1.056 | 3.575 | 2.652 |

[Full native confirmation samples and ABI reports](host-performance-master-activities-ci.json)
retain all 21 trials per pair, hashes and toolchain flags. Every report now
captures current revision `a0d1fdaf0c9e7524e85f2ab5213164ab6383723a`, including
Windows; the previous missing revision remains unchanged in its historical
report. Baseline revisions, workload hashes and within-host compiler/Release
flags match. Windows workload hashes reflect its CRLF source checkout.

Block/allocation ratios are lower than the earlier CI observations, consistent
with the local repair measurements. The CI runs use separate runner instances,
so they do not establish an exact isolated before/after speedup for this change;
the local same-host pair supplies that evidence. All hosts still show residual
slowdowns against pre-Phase-2, and Windows images remain about 6% slower in this
short workload. Native confirmation is complete; aggregate temporary allocation
and shared storage registry overhead remain open investigation targets.

### Aggregate temporaries and registry transactions

The subsequent repair keeps unmanaged aggregate temporaries on the stack when
their actual bounds and element stride are integer literals, the checked byte
count fits signed 64-bit arithmetic, and the temporary fits the existing
65,536-byte stack limit. The temporary remains separate and zeroed, including
the one-byte allocation for a null aggregate. Components may read the old
destination or raise; the destination is copied only after successful filling.
Dynamic/large/packed temporaries retain arena storage, while managed/limited
construction retains its existing ownership path. Choice, bounds and sliding
validation still precede component evaluation.

Owned deallocation now combines registry lookup and removal under one storage
mutex acquisition. Owned records still take precedence over borrowed views;
deallocating a borrowed-only view still raises Program_Error. The record remains
alive through cleanup, and joining tasks, finalizing objects and invoking user
pool callbacks happen after unlocking. Registry synchronization is retained.
No runtime layout or calling ABI changes in this repair.

Two staged native Darwin arm64 Release builds compare with the matching
compiler/runtime at `30d0ae995b65b3bd0933919ad8b5acb11a6b214e`. The aggregate-only
pair improved the block workload by about 55%, leaving allocation unchanged.
After the registry change, the combined 21-trial pair records:

| Workload | Before median (s) | Combined repair median (s) | Repair / before | Repair / pre-Phase-2, separate pair |
| --- | ---: | ---: | ---: | ---: |
| Scalar calls | 0.014276 | 0.014388 | 1.008 | 0.751 |
| Images | 0.024528 | 0.024584 | 1.002 | 0.785 |
| Block scopes | 0.017230 | 0.007837 | 0.455 | 1.620 |
| Allocation/deallocation | 0.019229 | 0.018581 | 0.966 | 2.674 |

[All staged/historical samples, component/source hashes and build flags](host-performance-aggregate-registry.json)
are retained. Comparisons alternate validated binaries on the same host, with
matching toolchain/Release options and no concurrent build/test load. The roughly
3% allocation difference is much smaller than the block improvement; longer
workloads are needed before a general conclusion. Process startup is included.

Both full Release stages pass 1,942/1,942 checks, with no failures or skips.
Fifteen ASan/UBSan and ten TSan checks pass for aggregate construction, generated
tasking, shared collections, borrowed storage, pool callbacks and failure cleanup.
The new `aggregatetemporaries` fixture exercises repeated self-referential
assignment, null aggregates, and a component exception after producing values
different from the old destination; it runs normally, deterministically, and in
whole/per-unit modes. `codegen.aggregate_storage` rejects arena-allocation calls
in that small-static fixture and fails against the preceding compiler as a
regression control. The existing hello IR reference is unchanged.

Native CI now confirms these two repairs at revision
`f9573946610d838aa0a418a9a01fc7ad21c75a14`. Both Ubuntu architectures pass
1,942/1,942 tests and MSYS2 passes 1,941/1,941, with no failures, disabled tests
or skips. All four new aggregate controls and the pool, shared-collection, ABI
and relocation checks pass. ABI reports are unchanged, including the 16-byte
master. The paired reports use matching flags/toolchains, the pinned historical
baseline and verified workload hashes (CRLF checkout on Windows).

| Host | Scalar calls, current / historical | Images | Blocks | Allocation |
| --- | ---: | ---: | ---: | ---: |
| Ubuntu amd64 | 0.705 | 0.850 | 1.941 | 4.712 |
| Ubuntu arm64 | 0.916 | 0.917 | 1.966 | 5.667 |
| MSYS2 UCRT64 amd64 | 0.703 | 0.882 | 1.549 | 2.739 |

[Full native confirmation samples](host-performance-aggregate-registry-ci.json)
retain all 21 trials per pair, revisions, flags and hashes. Earlier CI reports
remain separate. Block ratios are substantially lower than the previous CI
observations, consistent with the local aggregate repair. Allocation remains
the larger gap, and separate runner instances do not isolate an exact speedup
for the registry change. Windows images are faster than baseline in this run;
the earlier small slowdown did not reproduce, reinforcing the need for longer
workloads before attributing small differences.

Native confirmation is complete. Remaining investigation targets include
constant aggregate shape/copy helper calls and
per-allocation metadata allocation, shared registry indexing and open checks.
These costs are visible in the implementation but have not each been isolated
by measurement. Blocks/allocation remain slower than pre-Phase-2; the overall
no-slowdown goal remains open.

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
