# Host and target validation

Phase 8 has successful native **Darwin arm64, Linux amd64/arm64 and Windows
amd64 (MSYS2 UCRT64)** evidence. The latest CI runs pass 1,946/1,946 tests
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
| Linux amd64 | ubuntu-24.04, lib64 | Native CI: 1,946/1,946 PASS |
| Linux arm64 | ubuntu-24.04-arm, lib | Native CI: 1,946/1,946 PASS |
| Darwin amd64 | Manual validation | Unvalidated; no CI job |
| Darwin arm64 | Local development, lib/lib64 | Local native pass; no CI job |
| Windows amd64 | windows-2025, MSYS2 UCRT64/GCC, lib | Native CI: 1,945/1,945 PASS; all eight fixes confirmed |

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
