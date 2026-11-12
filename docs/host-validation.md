# Host and target validation

For the Phase 12 CXDA003 increment, the user confirms that CI builds and tests
pass on all configured platforms. Local Darwin arm64 validation passed all
2,197 Release checks (314 ACATS) and 36 focused ASan/UBSan checks.
[Hosted ceiling-locking contract and capability checks](ceiling-locking.md).
This CI confirmation does not supply per-job counts or native-priority skip
details, and does not independently establish privileged Linux FIFO/RR execution.

The detailed counts and pinned artifacts below describe earlier milestones.

Phase 8 has successful native **Darwin arm64, Linux amd64/arm64 and Windows
amd64 (MSYS2 UCRT64)** evidence. The recorded Phase 8 CI runs pass 1,946/1,946 tests
on each Ubuntu architecture and 1,945/1,945 on Windows, with no failures,
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
| Linux amd64 | ubuntu-24.04, lib64 | Native CI: 1,953/1,953 PASS |
| Linux arm64 | ubuntu-24.04-arm, lib | Native CI: 1,953/1,953 PASS |
| Darwin amd64 | Manual validation | Unvalidated; no CI job |
| Darwin arm64 | Local development, lib/lib64 | Local native pass; no CI job |
| Windows amd64 | windows-2025, MSYS2 UCRT64/GCC, lib | Native CI: 1,952/1,952 PASS; all eight fixes confirmed |

Runner labels follow GitHub's [native runner inventory](https://docs.github.com/en/actions/reference/runners/github-hosted-runners).
Windows uses the documented [MSYS2 UCRT64 setup](https://github.com/msys2/setup-msys2).
The build/test workflow preserves JUnit, CTest diagnostics, relocation logs and
ABI facts and the tested commit SHA (`revision.txt`) per runner. It does not
build the performance baseline or collect timings. The user supplied successful native
Linux/MSYS2 rerun artifacts. Linux has one more test overall: its POSIX host and
terminal checks are replaced by the native Windows host-library check on MSYS2.
Cross-assembly controls elsewhere in the suite are not native execution evidence.
The driver continues to target its build host/toolchain; no cross-compilation
option, 32-bit target or big-endian target is added.

## Routine tests and manual profiling

[Build and Test](../.github/workflows/build-and-test.yml) retains its push-to-master,
pull-request and manual triggers. It builds Release and runs the full native
suite, including ABI, installation and measurement-tool protocol tests. Those
protocol tests validate the tooling without collecting performance samples.

[Build and Profile](../.github/workflows/build-and-profile.yml) runs only through
`workflow_dispatch`; it has no push or pull-request trigger. Use local matched
profiling for routine tuning and run native profiling at milestones, or sooner
for ABI, platform, tasking or lifetime changes. Both workflows retain Ubuntu
amd64/arm64 and MSYS2, with macOS entries commented for consistency.

The manual workflow builds the candidate compiler/runtime, ABI probe and context
benchmark, plus a separately checked-out matching baseline compiler/runtime. It
runs the unchanged extended 21-case/11-pair suite without repeating the full
CTest suite. Set `baseline_ref` to the pre-batch commit for a same-run comparison
on the allocated CPU. The default remains historical pre-Phase-2 revision
`3d2aac5980f57fb68524a15b154f65cf0882bdc2` for milestone tracking. A branch/tag
input is resolved to its actual checked-out commit SHA in the report.

Validation artifacts keep the `host-*` prefix; measurement artifacts use
`profile-*` and contain `abi.json`/`timings.json`. Both may be downloaded into
`build/ci-artefacts`; test-only runs contain no timing report. Previously archived
profiling reports remain separate when test-only artifacts replace the download
folder. Workflow/YAML
and shell checks are local; the new manual workflow has not yet run on GitHub.

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

The initial paired CI workflow checked out the pinned pre-Phase-2 revision
with its QBE submodule and built its `ada` target separately in Release. Native
suites ran against current code; timing collection alternated current and
historical binaries for 21 trials. The extended protocol below supersedes that
short-only collection. The supplied Linux/MSYS2 artifacts confirm successful historical builds and
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

### Default-pool coallocation and constant aggregate sizes

Direct built-in default-pool collection allocations now reserve the zeroed
object and an aligned registry-record footer with one `calloc`. The object
remains the allocation base, and zero-sized objects still reserve one real byte.
Size/padding/record arithmetic is checked before allocation. Registry membership
remains locked; the footer survives task joining, finalization and arena cleanup,
then the block is freed once. User-selected pools, borrowed views and deferred
result construction keep their separate storage paths. A closing-pool callback
test verifies that rejected registration releases the returned object exactly
once through its original pool, preserving the pending exception.

For small unmanaged aggregates already proven to fit stack storage, the emitter
reuses the computed byte count instead of calling `__ada_array_size` again. It
retains an explicit pending-exception check before the final copy: temporary
component finalizers can raise even after producing their scalar value. Dynamic,
large, packed and managed paths keep their existing checks and ownership.

Native Darwin arm64 Release pairs compare the working tree with matching builds
of `e5d98f58363c6780b1351f9a3aaf16ae31dd6299` and, separately, pre-Phase-2. Each
pair alternates validated binaries for 21 trials on the same idle host/toolchain.
Compilation and tests finish before timing; process startup remains included.

| Workload | Before median (s) | Repair median (s) | Repair / before | Repair / pre-Phase-2, separate pair |
| --- | ---: | ---: | ---: | ---: |
| Scalar calls | 0.014064 | 0.014089 | 1.002 | 0.766 |
| Images | 0.024220 | 0.024320 | 1.004 | 0.785 |
| Block scopes | 0.007589 | 0.007017 | 0.925 | 1.474 |
| Allocation/deallocation | 0.018360 | 0.016289 | 0.887 | 2.420 |

[All samples, component/source hashes, build flags and validation](host-performance-coallocation.json)
are retained. Allocation improves about 11% and blocks about 7.5% against the
preceding build; scalar/image differences are small. The remaining historical
gaps are substantial, so this is not a no-slowdown conclusion.

The final full suite passes 1,945/1,945 checks, with no failures or skips.
Eighteen ASan/UBSan and 13 TSan checks pass. Allocation instrumentation permits
exactly one allocation for zero, odd and larger default-pool object sizes,
checks alignment/zero initialization and verifies allocation-free release and
failure rollback. The failure sweep now continues to its first complete success
instead of assuming a fixed number of allocation points. Custom-pool callback
closure and reentry remain covered.

`aggregatetemporarycleanup` checks a failing finalizer on the last component's
limited controlled temporary, an unchanged destination, cleanup counts and a
subsequent successful assignment in normal/determinism/whole/per-unit modes.
The strengthened `codegen.aggregate_storage` rejects redundant size-helper calls
as well as arena allocation. The preceding compiler/runtime fail the new
size-helper and single-allocation controls respectively. The new raw-mode case
includes the Windows stack probe; public runtime layouts remain unchanged.

Native CI now confirms this repair at revision
`b067332b07ec37154bfb1d0bc7b4d1068bb415ee`. Both Ubuntu architectures pass
1,945/1,945 checks; MSYS2 passes 1,944/1,944, with no failures, disabled tests or
skips. Single-allocation, closing-pool and last-component temporary-finalizer
controls pass, as do ABI and relocated-install checks. Reported layouts remain
unchanged, including the 16-byte master.

| Host | Scalar calls, current / historical | Images | Blocks | Allocation |
| --- | ---: | ---: | ---: | ---: |
| Ubuntu amd64 | 0.597 | 0.675 | 2.211 | 5.669 |
| Ubuntu arm64 | 0.917 | 0.920 | 1.966 | 5.046 |
| MSYS2 UCRT64 amd64 | 0.701 | 0.870 | 1.495 | 2.347 |

[Full native confirmation reports](host-performance-coallocation-ci.json)
retain all 21 trials, revisions, component hashes and matching within-host
toolchain/Release flags. Workload source hashes match, allowing for CRLF checkout
on Windows. Earlier reports remain separate.

These short CI pairs do not establish a uniform improvement over the preceding
repair: Ubuntu amd64 block/allocation ratios are worse than the previous run,
while allocation ratios are lower on arm64 and MSYS2. Different runner instances
and short startup-inclusive workloads prevent attributing those cross-run
differences to this repair alone. The controlled local pair remains evidence
for that host; the native results establish correctness, not a broad speedup.
Historical block/allocation gaps remain on every host.

Native functional confirmation is complete. Longer workloads and batches
with multiple live objects are the next measurement targets: the current
allocation workload releases each object immediately and does not characterize
the linked registry's scaling. Remaining aggregate helper/copy and registry costs
need measurement before further tuning. The overall performance item stays open.

### Extended and batched measurements

Schema 3 adds an extended suite while retaining the four original short inputs
byte-for-byte. Long cases wrap each unchanged kernel in a nested procedure and
invoke it 128 times in one process, validating every activation. This retains
each kernel's initialization/cleanup and adds wrapper activation/static-link
structure; compare each long case with its historical counterpart rather than
pooling its timings with the old top-level short case.

The suite has 21 cases: four short kernels, an empty-program startup control,
four long kernels, nine live-set scaling cases and three long batch-throughput
cases. Scaling holds total object allocations at 65,536 while varying the live
set across 32, 512 and 4,096 objects and release order across FIFO, LIFO and a
coprime permutation. Throughput holds 32 objects live for 4,194,304 allocations
per order. Each batch checks its checksum and cleared access values. Its pointer
table lives on the heap, avoiding a new large stack frame in the historical
Windows compiler. Input limits keep the permutation and checksum arithmetic
valid; every compiler/program invocation retains a finite deadline.

Generated Ada sources, source/kernel/template/executable/collector hashes, exact
parameters, execution order, every sample, paired ratios and median absolute
deviation are retained. CPU descriptions/counts/affinity are recorded where
available, without promising identical hardware across runner instances.
Startup remains included; the startup control is never subtracted. A preliminary
32-repeat run left the fastest historical long block case around 41 ms, so the
final protocol uses 128 repeats and dedicated long-throughput batches.

[Full local reports, generated sources and protocol validation](host-performance-extended-darwin-arm64.json)
preserve both the preliminary and final runs. Native Darwin arm64 Release uses
compiler source revision `eec5df13c5e34b2806586ad7f24cada5b62bff28` (production
code unchanged since `b067332b07ec37154bfb1d0bc7b4d1068bb415ee`) and the pinned
pre-Phase-2 baseline, with matching flags/toolchains and 11 alternating trials.
Both compiler/runtime builds validate all 21 cases. No compiler/runtime tuning
is included in this measurement change.

| Repeated kernel | Current median (s) | Historical median (s) | Current / historical |
| --- | ---: | ---: | ---: |
| Scalar calls | 1.265531 | 1.857699 | 0.681 |
| Images | 2.552664 | 3.432846 | 0.744 |
| Block scopes | 0.401992 | 0.149389 | 2.691 |
| Allocation/deallocation | 1.564795 | 0.385394 | 4.060 |

The empty-program medians are 0.003562/0.003230 seconds (current/historical).
That control is a large fraction of the short baseline block time, but roughly
2% of the long baseline block time. It is a diagnostic, not an exact startup
cost model for every linked program. Long kernel/throughput median absolute
deviations are below 0.5% of their medians in this local run; this is observed
dispersion, not a confidence interval or a cross-host guarantee.

| Live objects, 65,536 allocations total | Current FIFO (s) | Current LIFO (s) | Current permuted (s) |
| --- | ---: | ---: | ---: |
| 32 | 0.016008 | 0.013571 | 0.015598 |
| 512 | 0.075078 | 0.013984 | 0.052359 |
| 4,096 | 1.057078 | 0.013892 | 0.863151 |

The historical scaling cases remain around 5–6 ms, where startup is still a
large fraction; avoid treating the large cross-build ratios as isolated registry
costs. Within current code, the live-set/order pattern is consistent with the
linear traversals in `findAllocationLocked` and `unlinkAllocation`. These cases
also include allocation, array access, index arithmetic and cleanup, so they do
not attribute all time to those helpers.

| 32-live throughput, 4,194,304 allocations | Current median (s) | Historical median (s) | Ratio |
| --- | ---: | ---: | ---: |
| FIFO | 0.760860 | 0.121326 | 6.271 |
| LIFO | 0.619334 | 0.143738 | 4.309 |
| Permuted | 0.718148 | 0.169991 | 4.225 |

Seven Python measurement protocol controls pass, including invalid configurations,
unchanged short inputs, generated-source/metadata agreement, failed/noisy runs
and finite deadlines. Both host protocol CTests pass. Small integration runs and
the final full extended suite validate generated families/orders with both
compilers. Compiler/runtime code is unchanged, so the full compiler suite was
not repeated for this tooling change.

All active CI jobs now collect this 21-case, 11-trial protocol using the existing
historical checkout. Their artifact names and `host evidence/timings.json` paths
are unchanged; reports use schema 3. macOS entries stay commented out. Native
Linux/MSYS2 confirmation now passes at revision
`62540d0fdd5204ed8c2bce91dcc3cb63d242cbbd`: Ubuntu amd64/arm64 each pass
1,946/1,946 tests; MSYS2 passes 1,945/1,945, without failures, disabled tests or
skips. Every host supplies all 21 validated cases and 11 sample pairs. Generated
sources, original kernel/template/collector hashes (including Windows checkout
line endings), parameters, medians, dispersion and execution order agree with
the protocol. Within-host compiler/Release configurations match, and ABI and
relocated-install checks remain unchanged.

| Host | Long scalar ratio | Long image ratio | Long block ratio | Long allocation ratio |
| --- | ---: | ---: | ---: | ---: |
| Ubuntu amd64 | 0.696 | 0.822 | 2.434 | 5.303 |
| Ubuntu arm64 | 0.923 | 0.920 | 2.384 | 5.959 |
| MSYS2 UCRT64 amd64 | 0.602 | 0.990 | 1.971 | 3.874 |

Ratios compare current/historical medians for the same generated long input.
Startup-control medians are about 0.5–0.7 ms on Ubuntu and 3.3–4.2 ms on MSYS2;
the historical long block medians are 122–181 ms. Startup remains included and
is not subtracted. These generated long cases should not be pooled with the
earlier short-case observations.

| Host, 4,096 live objects and 65,536 allocations | Current FIFO (s) | Current LIFO (s) | Current permuted (s) |
| --- | ---: | ---: | ---: |
| Ubuntu amd64 | 0.401102 | 0.008314 | 0.392330 |
| Ubuntu arm64 | 0.863443 | 0.013009 | 0.767055 |
| MSYS2 UCRT64 amd64 | 0.816080 | 0.018930 | 0.624698 |

All three hosts reproduce the live-set/order pattern: FIFO/permuted release
grows sharply from 32 to 4,096 live objects while LIFO stays comparatively flat.
At 4,096 objects FIFO is about 43–66x current LIFO within the same host/run.
The long 32-live throughput ratios also retain historical gaps: FIFO
8.200/7.608/5.419, LIFO 4.887/5.573/3.491 and permuted 4.818/5.425/3.633
on Ubuntu amd64/arm64/MSYS2 respectively.

[Full unmodified native reports](host-performance-extended-ci.json) retain every
generated input and sample, executable/build hashes, CPU descriptions and
parameters. Prior local and native evidence remains separate. The consistent
order/scaling result supports investigating the linked lookup/unlink paths;
it does not isolate every array, arithmetic, allocation or cache contribution.
Native extended confirmation is complete. The next optimization target is non-LIFO
lookup/unlink scaling, retaining ownership precedence, locks and callback rules.
The longer block/allocation gaps keep the overall no-slowdown goal open.

## Allocation registry indexing and direct unlinking

The next repair replaces address scans with an optional hash index from 64 live
records, and adds backward links for direct collection/global unlinking. Bucket
order follows global registration order, including deferred publication, retaining
owned-over-borrowed lookup and identity/rank view selection. Failed index enable
or growth preserves the list or old table without failing object allocation.
Small registries allocate no index; emptying a registry releases its table.
Callbacks, finalizers and joins remain outside the storage mutex.

The private allocation record grows from 104 to 120 bytes on Darwin arm64, and
private context storage from 880 to 896 bytes. Successful table growth retains a
power-of-two bucket array with at least one bucket per live record, plus its
capacity word. No shrinking occurs before the registry empties. Generated-code
ABI offsets and the host layout probe remain unchanged. Deferred publication and
finalization-owner classification can still walk the global list; collisions and
same-address aliases can still extend bucket traversal.

Final local Release validation passes 1,946/1,946, with no failures, disabled
tests or skips. Twenty ASan/UBSan and eight TSan focused checks pass. Controls
cover 600 live records, resizes, arbitrary release order, owned/borrowed and array
views, reverse deferred publication, optional-index allocation failure/recovery,
complete reclamation, concurrent collection workers and reentrant finalizers.
ABI, mixed-mode and relocated-install checks pass in the full suite.

Matched Release comparisons against preserved revision
`ca988b1418139eea1d4b2e9a706f878e9d4cbd6b` use the unchanged extended collector,
21 cases, 11 alternating pairs, one warmup, 128 long-kernel activations and the
same allocation counts as native CI. Medians include startup without subtraction.

| Workload | Before (s) | Indexed (s) | Indexed / before |
| --- | ---: | ---: | ---: |
| 4,096-live FIFO, 65,536 allocations | 1.042738 | 0.013983 | 0.0134 |
| 4,096-live LIFO, 65,536 allocations | 0.013686 | 0.014073 | 1.0282 |
| 4,096-live permuted, 65,536 allocations | 0.846452 | 0.013950 | 0.0165 |
| 32-live FIFO, 4,194,304 allocations | 0.760315 | 0.700675 | 0.9216 |
| 32-live LIFO, 4,194,304 allocations | 0.614635 | 0.628471 | 1.0225 |
| 32-live permuted, 4,194,304 allocations | 0.716021 | 0.690665 | 0.9646 |
| Long blocks | 0.403005 | 0.403230 | 1.0006 |
| Long allocation | 1.567601 | 1.620445 | 1.0337 |

FIFO/permuted large-live-set timings improve about 75x/61x. Current FIFO is
approximately equal to current LIFO, removing the measured order-dependent
scaling in these workloads. Costs remain visible: long allocation is about 3.4%
slower, long 32-live LIFO throughput about 2.3% slower and 4,096-live LIFO about
2.8% slower. Long block timing is essentially unchanged. The initial implementation
also called `free(NULL)` when emptying an unindexed registry; removing that call
reduced the observed long allocation cost before the final validation/measurements.
These whole-program timings include array/index work and do not isolate every
helper or memory-layout contribution.

[Full final local reports](host-performance-allocation-index.json) retain source,
build, collector and executable hashes, every sample, dispersion, order and
parameters. Native confirmation is recorded below; macOS workflow entries
remain commented.
The separate matched comparison against pre-Phase-2 revision
`3d2aac5980f57fb68524a15b154f65cf0882bdc2` also validates all 21 cases over
11 pairs with the same final current compiler/runtime and inputs. Long blocks
remain 2.676x historical and long allocation 4.174x. At 4,096 live objects,
current FIFO/LIFO/permuted take 0.014532/0.013771/0.014470 seconds versus
0.005536/0.005716/0.006208 historically. The repair addresses order-dependent
scaling; it does not achieve the overall historical no-slowdown goal.

## Indexed registry native CI confirmation

The user-provided native artifacts for revision
`0a6ff951bf230c8430eba83e6bb2c9c677090b1d` pass 1,946/1,946 on each Ubuntu
architecture and 1,945/1,945 on MSYS2 UCRT64, with no failures, disabled tests
or skips. Allocation/index failure, alias/publication, concurrent collection,
custom pool and measurement protocol controls pass. ABI probes remain 64-bit
pointers, 24-byte traces, 48-byte array results and 16-byte masters; Windows
retains its 32-bit C `long`. Mixed ABI and relocated installations pass,
including spaced paths, library/runtime overrides and native path handling.

All 21 cases validate over 11 alternating pairs per host, with one warmup per
program. Build configurations match within each pair. Parameters, generated
sources, medians, MADs, paired ratios and execution order verify. Windows
collector and checked-in input hashes match the CRLF checkout; generated inputs
match LF bytes. The source content agrees with the local measurement suite.
Startup remains included, without subtraction.

| Host | 4,096-live FIFO (s) | LIFO (s) | Permuted (s) | FIFO / LIFO | Long block / historical | Long allocation / historical |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Ubuntu amd64 | 0.009385 | 0.009204 | 0.009596 | 1.020 | 2.419 | 5.949 |
| Ubuntu arm64 | 0.016595 | 0.016932 | 0.017461 | 0.980 | 2.408 | 6.797 |
| MSYS2 UCRT64 | 0.027268 | 0.028242 | 0.028859 | 0.966 | 2.009 | 3.424 |

The three release orders now take similar time on every host, confirming the
repair of measured non-LIFO scaling. These CI pairs compare the indexed build
with pre-Phase-2 revision `3d2aac5980f57fb68524a15b154f65cf0882bdc2`, rather
than the immediately preceding implementation. Earlier CI reports remain
separate: their 43–66x FIFO/LIFO gap is absent here, but changes between runs
cannot isolate this repair's whole-program cost. Historical long allocation
ratios are higher on both Ubuntu hosts and lower on MSYS2 than the preceding
CI run; this is not a uniform end-to-end speedup. The matched local preceding-build
comparison above retains the small allocation/LIFO regressions.

[Complete native timing reports](host-performance-allocation-index-ci.json)
preserve every sample, generated input, executable/build hash, host description
and parameter. [Pinned test/install/ABI summaries](host-validation-ci.json)
retain the current run and 21 earlier host records. `build/ci-artefacts` can be
overwritten again. Indexed-registry native confirmation is complete; the next
step is profiling the remaining long block/allocation overhead before choosing
another repair. The historical no-slowdown goal remains open.

## Remaining overhead profiling

[Local diagnostic profiling](runtime-profiling.md) records two 11-trial studies
and independent untimed source-entry censuses without modifying production
compiler/runtime builds. The allocation kernel makes about 17 mostly-empty
finalization entries per object; guarding those calls reduces diagnostic timing
about 27%. Master omission reduces blocks about 34%, but requires ownership and
accessibility proof. The selected next repair is a small empty-chain finalization
entry before TLS/large-frame setup. Full reports, controls, reproduction and
limits are retained separately. The existing 21-case suite remains the gate for
any production repair; historical no-slowdown remains open.

## Empty finalization entry path

The selected repair moves the existing nonempty finalization algorithm into a
private `finalizeNonempty` helper. Public `__ada_finalize_to` checks owner against
checkpoint before calling it. A deliberate GCC/Clang `noinline` attribute keeps
the saved-occurrence frame out of the empty path; an ordinary static helper was
inlined by Apple clang and retained the heavy frame despite the earlier test.

Matching Release `-O3 -DNDEBUG -fPIC` arm64 assembly now has only a load, compare,
conditional branch, tail branch to the helper and return in the public entry.
The empty path has no TLS resolution, stack guard, spills or stack allocation,
compared with the former 656-byte frame and TLS lookup before the comparison.
This assembly observation is local to Apple clang; native functional and timing
confirmation is recorded below. No native GCC assembly capture is claimed.

The nonempty algorithm, public ABI, allocation registry, locking and callback
rules are unchanged. New task-context controls check null and non-null empty
checkpoints with a pending message/trace, active record and enclosing abort
deferral, then verify the same record's nonempty cleanup and occurrence restore.
Local Release passes 1,946/1,946, with no failures, disabled tests or skips;
20 ASan/UBSan and eight TSan focused checks pass. Mixed ABI, layout and relocated
installation checks pass. Layout remains 24-byte traces, 48-byte array results
and 16-byte masters.

The matched comparison uses preserved Release revision
`33320aa533d4a2522db8800ae7469592e9fb2ef0`, unchanged inputs/collector and the
same extended protocol: 21 cases, 11 alternating pairs, one warmup, 128 repeated
kernel activations, 65,536 allocations at 32/512/4,096 live objects and
4,194,304 allocations for 32-live throughput. Startup is included, not subtracted.

| Workload | Before (s) | Small entry (s) | After / before |
| --- | ---: | ---: | ---: |
| Long blocks | 0.407019 | 0.407936 | 1.0023 |
| Long allocation | 1.633098 | 1.307821 | 0.8008 |
| 32-live FIFO throughput | 0.710335 | 0.567149 | 0.7984 |
| 32-live LIFO throughput | 0.638267 | 0.486539 | 0.7623 |
| 32-live permuted throughput | 0.703403 | 0.558176 | 0.7935 |
| 4,096-live FIFO | 0.014164 | 0.011827 | 0.8350 |
| 4,096-live LIFO | 0.013903 | 0.011521 | 0.8286 |
| 4,096-live permuted | 0.014451 | 0.012483 | 0.8638 |

The long allocation workload improves about 20%, and 32-live throughput about
20–24%, without reintroducing the measured non-LIFO scaling. Long blocks remain
essentially unchanged, consistent with the diagnostic census. These are whole
program measurements, including code-layout effects; the diagnostic 27% guard
result was not a prediction of an identical runtime-wrapper gain.

[Complete final local reports](host-performance-finalization-entry.json) preserve
all samples, generated inputs, hashes, flags, order, dispersion, validation and
before/after assembly entries. Native Ubuntu amd64/arm64 and MSYS2 confirmation
is recorded below; macOS workflow entries remain commented. A separate matched comparison with pre-Phase-2 revision
`3d2aac5980f57fb68524a15b154f65cf0882bdc2` validates the same 21 inputs over
11 pairs with the identical final current compiler/runtime hashes. Long blocks
remain 2.692x historical and long allocation 3.345x; 32-live LIFO throughput
remains 3.340x. At 4,096 live objects FIFO/LIFO/permuted take
0.011764/0.011406/0.012137 seconds versus 0.005267/0.005804/0.005991
historically. Reports stay separate rather than pooling their medians. The
historical no-slowdown goal remains open.

## Empty finalization entry native CI confirmation

Revision `4711d164d76344320b0fcb4aa191d7740852f184` passes 1,946/1,946 tests
on each Ubuntu architecture and 1,945/1,945 on MSYS2 UCRT64. There are no
failures, disabled tests or skips. The task-context controls for empty null and
non-null checkpoints, pending occurrences and enclosing abort deferral pass,
alongside reentrant/failing finalizer, shared collection and pool controls.
ABI probes and mixed-mode checks are unchanged: 24-byte traces, 48-byte array
results, 16-byte masters, 64-bit pointers and the expected Windows 32-bit C
`long`. Relocated installations, spaced paths and library/runtime overrides pass.

All 21 cases validate over 11 alternating pairs per host. Release configurations
match within pairs; input parameters/content, samples, medians, MADs, paired
ratios and execution order verify. Windows collector and checked-in input hashes
match the CRLF checkout; generated source hashes match LF bytes. Startup is
included without subtraction. Native pairs compare with pre-Phase-2 revision
`3d2aac5980f57fb68524a15b154f65cf0882bdc2`.

| Host | Long block / historical | Long allocation / historical | 32-live LIFO throughput / historical | 4,096-live FIFO / LIFO |
| --- | ---: | ---: | ---: | ---: |
| Ubuntu amd64 | 2.448 | 4.787 | 4.504 | 1.065 |
| Ubuntu arm64 | 2.399 | 5.454 | 5.299 | 0.961 |
| MSYS2 UCRT64 | 1.974 | 2.846 | 2.632 | 0.985 |

Historical long allocation ratios are lower on all three hosts than in the
preceding indexed-registry CI report (5.949/6.797/3.424). Their normalized ratios
are about 17–20% lower, consistent with the local matched repair result. These
are separate CI runs; VM, toolchain and timing variation prevent attributing an
exact isolated speedup to the repair from that comparison. The matched local
preceding-build evidence remains separate. Long block ratios remain near their
previous values, and the large-live-set release-order gap stays absent.

[Complete native timing reports](host-performance-finalization-entry-ci.json)
preserve every generated input, sample, hash, host description, parameter and
order. [Pinned test/install/ABI summaries](host-validation-ci.json) retain this
run and 24 earlier host records. `build/ci-artefacts` can be overwritten again.
No compiler/runtime changes were made during this artifact review; full local
suites were not repeated for documentation and evidence updates.

Native confirmation of the finalization entry repair is complete. Historical
block/allocation gaps keep the no-slowdown goal open. The next investigation is
conservative empty lexical-master lowering: the diagnostic benefit does not
establish a safe optimization for arbitrary ancestry, accessibility, collection
ownership, dependent tasks or exceptional/finalization paths. Keep hooks where
those properties cannot be proved.

## Conservative lexical-block master elision

The measured empty-block cost now has a conservative compiler repair, documented
in the [master-observation proof](lexical-master-elision.md). All source/foreign/
indirect and unclassified runtime calls default to potentially observing master
ancestry. Only audited primitive emission sites are independent. Candidate
lifecycle hooks have their own classification. No purity is inferred from symbol
text; a source import named `__ada_array_offset` retains its master.

A block can omit its record/hooks only when no observing call is emitted across
its declarations, branches, nested blocks and exits. The evidence is monotonic
and independent of lexical-stack copies during transfer emission. The emitter
retains block depths, storage checkpoints, cleanup and exception checks, and
filters only exact proven record/hook lines after return plans are expanded.
Function/task/library/collection master policies and runtime sources are unchanged.

Two new fixtures run normally, deterministically and in whole/per-unit modes.
They cover nested primitive blocks, goto, early return, exception propagation,
source/imported calls and a pure child within an observing parent. The codegen
control also retains masters for controlled cleanup, tasking and anonymous access.
The preserved pre-change compiler runs both new programs successfully. Local
Release passes 1,953/1,953 with no failures, disabled tests or skips; 27 ASan/UBSan
and 15 TSan focused checks pass. Mixed ABI, layout and relocated installs pass.
The layout probe remains 24-byte traces, 48-byte array results and 16-byte masters.

Matched Release measurements use revision
`96413db3ae80312de26326e6df0930178bfa8c39` as the immediate baseline, unchanged
sources/collector, 21 cases, 11 alternating pairs, one warmup, 128 long-kernel
activations and the same live-set/allocation counts as CI. Startup is included
without subtraction. All 19 runtime object payloads are byte-identical between
these builds; their archive container hashes differ and are retained unmodified.
Main-unit SSA also matches exactly for scalar, image, allocation, long allocation
and the LIFO throughput control.

| Workload | Before (s) | Elided blocks (s) | After / before |
| --- | ---: | ---: | ---: |
| Short blocks | 0.007056 | 0.005622 | 0.7968 |
| Long blocks | 0.406865 | 0.266297 | 0.6545 |
| Long allocation | 1.307053 | 1.306801 | 0.9998 |
| 32-live FIFO throughput | 0.570814 | 0.575329 | 1.0079 |
| 32-live LIFO throughput | 0.487981 | 0.488558 | 1.0012 |
| 32-live permuted throughput | 0.557196 | 0.556747 | 0.9992 |
| 4,096-live FIFO | 0.011977 | 0.011919 | 0.9952 |
| 4,096-live LIFO | 0.011490 | 0.011488 | 0.9998 |
| 4,096-live permuted | 0.012157 | 0.012193 | 1.0030 |

Long blocks improve about 35%, consistent with the earlier diagnostic experiment.
Other long controls and throughput medians differ by less than 1% in this run.
The short 32-live permuted batch is 3.1% slower; its long throughput counterpart
is essentially unchanged. Keep all samples rather than claiming a uniform gain.
These whole-program differences include generated-code/layout effects.

A separate paired comparison uses pre-Phase-2 revision
`3d2aac5980f57fb68524a15b154f65cf0882bdc2` with identical final current build
components and inputs over all 21 cases/11 pairs. Long blocks remain 1.758x
historical, long allocation 3.319x and 32-live LIFO throughput 3.338x. Keep the two
studies separate; the overall historical no-slowdown goal remains open.

[Complete local reports](host-performance-block-masters.json) retain every input,
sample, executable/build/source hash, order, dispersion, proof boundary and
validation result. Native confirmation is recorded below; macOS workflow entries
remain commented.

## Lexical-block master native CI confirmation

Revision `a815b029c5c72d6b326b69c6ac0408ac1d051db1` passes 1,953/1,953 tests
on each Ubuntu architecture and 1,952/1,952 on MSYS2 UCRT64, with no failures,
disabled tests or skips. All seven new registrations pass: primitive-block and
source/imported-call fixtures in normal/deterministic/whole/per-unit modes, plus
the codegen retention control for source calls, managed cleanup, tasking and
anonymous access. Mixed ABI, layout and relocated-install checks pass. Layout
remains 24-byte traces, 48-byte array results and 16-byte masters, with the
expected native C widths.

All 21 timing cases validate over 11 alternating pairs per host with matching
Release configurations. Inputs/parameters, every sample, medians, MADs, ratios
and execution order verify. Windows collector and checked-in input hashes match
the CRLF checkout; generated sources match LF hashes. Startup is included without
subtraction. Each native pair uses the pre-Phase-2 baseline
`3d2aac5980f57fb68524a15b154f65cf0882bdc2`.

| Host | Long block / historical | Long allocation / historical | 32-live LIFO throughput / historical | 4,096-live FIFO / LIFO |
| --- | ---: | ---: | ---: | ---: |
| Ubuntu amd64 | 1.908 | 4.901 | 4.189 | 1.056 |
| Ubuntu arm64 | 1.735 | 5.454 | 5.302 | 0.962 |
| MSYS2 UCRT64 | 1.592 | 3.185 | 2.784 | 0.963 |

Block ratios are lower on every host than the preceding CI run
(2.448/2.399/1.974), consistent with the matched local block improvement. These
jobs do not provide a controlled pair against the immediately preceding compiler.
The amd64 runner changed from AMD EPYC 9V45 to 9V74, and Windows changed from AMD
to Intel (family/model descriptions are retained in the raw reports). The arm64
CPU implementer/part remains 0x41/0xd49. Release toolchain configurations agree
across the two runs, but CPU/VM differences can reweight even historical-normalized
ratios; do not pool wall times or assign an exact cross-run repair gain.

Allocation ratios are 4.901/5.454/3.185 versus 4.787/5.454/2.846 previously.
The roughly 12% higher Windows ratio is retained as variability requiring a
same-host controlled check before attribution, not dismissed as a proven noise
effect or treated as an isolated regression. Local allocation-unit SSA controls
remain byte-identical, but no native SSA identity claim is made from these
artifacts. Large-live-set release orders remain close; historical block and
allocation gaps remain open.

[Complete native reports](host-performance-block-masters-ci.json) preserve all
samples, sources, hashes, CPU descriptions and parameters. [Pinned validation
summaries](host-validation-ci.json) retain this run and 27 earlier host records.
`build/ci-artefacts` can be overwritten again. This review changes documentation
and retained evidence only; no local compiler/sanitizer rerun was needed.

Native proof/functional confirmation is complete. The next investigation is
remaining aggregate/array-index helper work and allocation overhead, with matched
same-host builds and all existing small/LIFO, throughput and scaling controls.
The historical no-slowdown goal remains open.

## Remaining helper diagnosis

[Local helper profiling](helper-profiling.md) now separates aggregate zeroing,
small copies, checked offsets and allocation call/lock overhead. The pinned-corpus
eight-block/seven-allocation-variant, 11-trial study selects equivalent inline
scratch zeroing as the next production target: it reduces diagnostic long block
time about 26%, versus 29% for omitting zeroing. Copy and offset probes reduce it
about 20% and 7%; gains are not additive.
Allocation guard/lock diagnostics retain their single-thread and callback limits.
The full 21-case/11-pair suite remains as an identical-build control, not a new
historical comparison; production component hashes are unchanged. Twenty
Python controls and both host protocol CTests pass. Full compiler/sanitizer suites
were not repeated for tooling only. Complete samples, fault parity and census
predicates are [archived](host-performance-helper-profile.json).

The next repair preserves all zero initialization while inlining small, known-size
scratch zeroing. A separate executable checks that all 16 bytes are cleared. Keep
source locations, scratch isolation, component checks and the pre-copy exception
barrier, with existing fallbacks for unsupported sizes/alignment and allocation. Local profiling remains
the routine path, with manual native profiling at milestones. Historical
no-slowdown remains open.

## Inline scratch zeroing

[Small aggregate scratch zeroing](inline-scratch-zeroing.md) now uses exact aligned
stores up to 32 bytes, retaining initialization, source locations, separate
scratch storage and the commit exception barrier. Null/tail/threshold/canary
controls and static/dynamic/large fallbacks pass. All 1,957 distinct local
correctness checks, 59 ASan/UBSan and 20 focused TSan checks pass; the report
records the corrected stale codegen test invocation from the full run.

The matched 21-case/11-pair production comparison uses identical backend/runtime
bytes and matching Release flags. Long block time falls 24.7% (0.266976s to
0.200988s); long allocation ratio is 0.9987. Scaling ratios are 0.9919–1.0185 and
throughput 0.9904–1.0007. [Full reports](host-performance-inline-zeroing.json)
retain the working-tree patch and all samples. Native correctness confirmation
remains for a routine CI checkpoint; native profiling remains manual. Residual
helper/allocation overhead and historical no-slowdown stay open.

## Post-zeroing residual diagnosis

[Local residual helper profiling](residual-helper-profiling.md) retains production
zeroing and measures offset/copy probes against that updated compiler. The final
seven-variant block study finds 8.2% lower diagnostic time for proven constant
offset folding, 6.7% for small copy and 27.7% together. Allocation controls remain
identical; the preceding 21-case report is retained with matching current
component hashes, rather than retimed. Twenty-three Python protocol checks and
both host protocol CTests pass; production compiler/runtime code is unchanged.

The selected next repair is proven-valid constant unpacked indexing offsets,
with helper fallback and the existing evaluation, location and exception contract.
[Raw reports](host-performance-residual-helpers.json) preserve samples, both study
matrices, census/fault controls and collector sources. Native checkpoints remain
pending while work continues locally; historical no-slowdown is still open.

## Optimization batch closed; functional work resumed

The [performance batch wrap-up](performance-wrap-up.md) implements constant
unpacked offsets with a proven valid result and bounded copies between known
stack buffers. Runtime fallback, source locations, exception barriers, exact byte
coverage and overlap semantics remain explicit. Further profiling is deferred at
the user's request; existing reports remain historical evidence, with no new
production speedup claim. The no-slowdown goal and native checkpoints stay open
without blocking functional work.

[Bound prefixed views](prefixed-views.md) now support renaming and generic actuals
for the documented tagged receiver surface; unchanged ACATS C413005 passes.
Array/runtime aggregate receiver layouts remain diagnosed and tracked separately.

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
