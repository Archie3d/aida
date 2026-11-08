# Task-context access ABI

Generated functions obtain `AdaTaskContext*` from their existing
`__ada_trace_enter` call and retain it until return. Pending exception identity
is the first pointer in the context (offset zero). The binder uses
`__ada_task_context` once at entry. This includes elaboration routines and
compiler-generated controlled-type helpers, which use the common function
emitter.

The trace frame remains 24 bytes; the location pointer is at offset 16.
Generated code updates that stack slot before potentially raising calls, then
uses `__ada_trace_leave_context` on every return after cleanup. C static
assertions protect these offsets. Context TLS is entirely inside C; generated
QBE uses no thread-data references or direct mutable runtime globals.

A context binding must survive a generated activation. C code may temporarily
bind another context for a callback provided it restores the previous binding
before returning. Each worker must bind its own context before calling Ada.
This ABI does not change Ada parameters, C imports, nested static links,
dispatch slots, finalizer callbacks, or owned-result descriptors. Recompile
existing Ada objects against the matching runtime.

## Why this convention

A separate accessor adds another call to every generated function. Folding
that lookup into traceback entry performs the same necessary work in one call.
A hidden context parameter would also require changing every Ada call path,
indirect-call descriptor/bridge, and runtime callback. The hidden-parameter
probe did not show a material speed benefit on the measured host, so that
additional ABI surface is unnecessary. The probe models entry cost only; it is
not a second implemented compiler ABI and excludes foreign-entry bridges.

## Reproduce the entry-cost comparison

Build the optional target with optimized C runtime code:

```sh
cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release
cmake --build build-bench --target context_access_benchmark -j 4
build-bench/tests/context_access_benchmark 0
build-bench/tests/context_access_benchmark 1
build-bench/tests/context_access_benchmark 2
```

Modes 0, 1 and 2 model separate accessor/entry calls, fused entry, and a hidden
context parameter respectively. Each prints seconds for ten million frame
entry/location-update/exit cycles. `ContextProbe.c` is compiled separately
without LTO so its hidden-entry code is not inlined into the loop. Alternate
run order and compare medians; timing is never a CTest pass/fail threshold.

On macOS arm64, using `cc -O2` for both probes and the runtime, eleven
alternating runs measured:

| Entry convention | Median seconds |
| --- | ---: |
| Separate accessor | 0.02832 |
| Fused traceback entry (chosen) | 0.02829 |
| Hidden parameter entry probe | 0.02895 |

The differences between these entry probes are small and host-specific. The
larger benefit comes from removing repeated trace-location and trace-exit TLS
lookups. Step 1's previous three-helper trace loop took 0.0534 seconds for ten
million cycles (0.0385 before contexts); this comparison is indicative because
the older loop did not contain the probe's mode switch.

## Generated Ada calls

`contextcalls.adb` checks two million calls to a nested scalar function. Compile
and link it with the compiler/runtime revision being measured, then alternate
executions of the two binaries. On this host, using QBE-generated code and the
default unoptimized CMake runtime, eleven runs gave process wall-time medians:

| Compiler/runtime | Median seconds |
| --- | ---: |
| Step 1, direct main-context loads and repeated C TLS lookups | 0.03156 |
| Cached context with direct trace-location stores | 0.01951 |

A separate comparison against the saved pre-context runtime used the Step 1 IR
with only the main-context data symbol changed back to the original exception
symbol (its remaining IR is unchanged). That comparison measured 0.02156
seconds before contexts and 0.01990 with this ABI. These short process timings
include startup and have occasional outliers; they are evidence for this
workload, not a universal performance guarantee.

Step 1 also measured 500,000 C-only exception raise-message/capture/arena-release
cycles at 0.0460 seconds before contexts and 0.0496 after contexts. This change
does not optimize those C-only helpers. The phase-wide no-slowdown target still
requires broader workloads and other target measurements.

## Master and activation boundaries

The original Phase 2 hooks reserved an eight-byte identity per emitted master.
The current record is 16 bytes, with atomic activity flags that allow empty
masters to skip shared-registry locking; compiler and runtime use the shared
size in `common/Master.h`. Hooks use the already-cached task context.
Empty subprogram boundaries are elided when
there are no declarations or owned temporaries; nested blocks still carry
their own hooks. This avoids extra calls on scalar leaf routines such as
`Next` in `contextcalls.adb`. Scopes with declarations do execute no-op hook
calls, so these measurements are not a claim of zero cost for every workload.

Compared with the pre-hook revision `ba9054c`, 21 alternating process runs on
macOS arm64 gave these wall-time medians, in seconds:

| Workload | Before master hooks | With master hooks |
| --- | ---: | ---: |
| `contextcalls.adb` | 0.02536 | 0.02547 |
| `imagecalls.adb` | 0.03939 | 0.03935 |

Both revisions used the default CMake build (no build type/optimization flags),
the same QBE executable and C linker, and their matching static runtimes. To
reproduce, build `adac` and `adart` at each revision, compile each benchmark
with `adac -o program.ssa source.adb`, run QBE on that IR, and link the assembly
with the corresponding `libadart.a` and `-pthread` (plus `-lm` on Linux).
Alternate binary execution order and compare medians; startup is included.
The sub-percent differences show no material regression in these probes.
Block-heavy, allocation-heavy and other-target measurements remain open.

## Caller-owned image results

`imagecalls.adb` performs 200,000 integer images and 200,000 fixed-point images,
passes each to a nested consumer, and checks the total length. Compile it with
the compiler/runtime revision being measured, then time alternating executions
of the resulting binaries. For the current version:

```sh
build/ada/ada -o build/imagecalls tests/benchmarks/imagecalls.adb
time build/imagecalls
```

On macOS arm64, using the default unoptimized CMake runtime and eleven
alternating runs, median process wall time was 0.03447 seconds with rotating
buffers and 0.03083 seconds with caller-owned buffers. The new path removes
fixed-point image allocation and uses returned lengths instead of generated
`strlen` calls. Scalar image storage is reserved in the caller's stack frame;
unconstrained stream results separately use the existing heap-transfer ABI.
This benchmark covers image generation, not stream allocation throughput, and
the short timings include process startup. The phase-wide performance caveats
above still apply.

## Shared-table locking

`SharedTables.c` measures 20,000 scratch-file write/reset/read/reset cycles,
then one million duplicate-tag registration/name-lookup pairs. It validates
the read data and lookup results. The optional target is built with:

```sh
cmake -S . -B build
cmake --build build --target shared_tables_benchmark
build/tests/shared_tables_benchmark
```

The two output columns are elapsed seconds for I/O and tags. On macOS arm64,
with the benchmark compiled at `-O2`, the default unoptimized CMake runtime,
and eleven alternating runs against the saved runtime from before locking:

| Workload | Before locks | With locks |
| --- | ---: | ---: |
| 20,000 I/O cycles | 0.05969 s | 0.06097 s |
| 1,000,000 tag register/lookup pairs | 0.00872 s | 0.01998 s |

This quantifies uncontended locking cost, not concurrent throughput. Tag lookup
cost rises measurably; the phase-wide no-slowdown goal is still open.

The blocking-I/O follow-up replaces the coarse I/O lock with per-file locks,
short table-lock intervals, and pins that protect slots while callers wait.
Eleven alternating runs under the same build settings compared the saved
coarse-lock binary with the new runtime:

| Workload | Coarse I/O lock | Per-file locks |
| --- | ---: | ---: |
| 20,000 I/O cycles | 0.06499 s | 0.06852 s |
| 1,000,000 tag register/lookup pairs | 0.01995 s | 0.01968 s |

Uncontended I/O costs about 5.4% more in this run; the tag implementation is
unchanged. The benefit is independent progress: `runtime.blocking_io` keeps
text, stream, and fixed-point readers blocked on pipes, queues Close or Reset
on that same file, and completes unrelated file operations before releasing
the reader. Same-file operations still serialize. These timings do not measure
contended throughput.

## Checked integer arithmetic

`checkedarithmetic.adb` runs ten million mixed add/multiply/rem iterations at
both signed widths. `checkedmultiply.adb` runs ten million long multiply/add/rem
iterations with a changing multiplier. Each validates an exact final checksum.
The operands in these workloads fit signed words, exercising the common
64-bit multiplication path; the general wide-product bound division, powers
and exception paths are not timed.

On macOS arm64, nine alternating before/after runs on 2026-10-22 measured the
following wall-time medians (process startup included):

| Workload | C helpers, development runtime | Inline, development runtime | C helpers, optimized runtime | Inline, optimized runtime |
| --- | ---: | ---: | ---: | ---: |
| Mixed 32/64-bit arithmetic | 0.37052 s | 0.10304 s | 0.18959 s | 0.10331 s |
| Changing long multiplier | 0.31726 s | 0.09257 s | 0.16169 s | 0.09245 s |

The baseline emitter is revision `3614c9c`; both compilers use the same QBE
backend and runtime for each pair. The development C runtime has no optimization
flag; the Release C runtime uses `-O3 -DNDEBUG`. Generated Ada code uses QBE's
normal optimization in both cases. No LTO is used. These workloads improve by
about 3.4–3.6 times with the development runtime and 1.7–1.8 times with the
optimized runtime. Timings are host- and workload-specific; in particular they
do not establish a speed improvement for general wide multiplication.

To reproduce, retain the previous `adac` executable before rebuilding the new
compiler, then build both workloads with each emitter. For example:

```sh
ADAC=/absolute/path/to/previous/adac build/ada/ada \
    -o /tmp/arithmetic-before tests/benchmarks/checkedarithmetic.adb
build/ada/ada -o /tmp/arithmetic-after tests/benchmarks/checkedarithmetic.adb
cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release
cmake --build build-bench --target adart -j 4
```

For the optimized-runtime pair, set `ADA_RUNTIME` to the absolute path of
`build-bench/runtime/libadart.a` on **both** compile commands. Repeat with
`checkedmultiply.adb`. Alternate run order nine times and take the median of
wall-clock durations from a monotonic timer. Timing is not a CTest threshold;
`runtime.checkedarithmetic_modes` separately tests results and exceptions and
rejects generated integer-helper references.

## Repeatable host/target collection

[Phase 8 host validation](../../docs/host-validation.md) adds a portable collector
for contextcalls, imagecalls, blockcalls (200,000 scopes) and allocationcalls
(100,000 allocations with explicit deallocation). Every program checks its
checksum. Timings include process startup and never become CTest thresholds.

```sh
cmake --build build --target context_access_benchmark
python3 -B tests/host/measure.py --ada build/ada/ada --build build \
    --context-probe build/tests/context_access_benchmark \
    --work build/host-evidence --runs 11
```

The JSON records host identity, individual samples, medians and hashes of the
workloads, driver, frontend, QBE, runtime and entry probe. Add `--baseline-ada`
and `--baseline-build` for a real alternating before/after comparison using
matching compiler/runtime builds on the same host/toolchain. The collector
rejects failed workloads; an identical-driver control is labeled as a harness
check. Current-only collection and entry models do not prove Phase 2's broad
no-slowdown goal. Native historical pairs now cover all active CI hosts;
[full cross-host samples](../../docs/host-performance-ci.json) preserve the
block/allocation regressions and the Windows revision-field limitation.

The CI workflow now builds the pinned pre-Phase-2 revision
`3d2aac5980f57fb68524a15b154f65cf0882bdc2`; the current extended protocol
collects 11 alternating trials per case (the earlier short pairs used 21).
Use `--revision` and `--baseline-revision` to record the corresponding compiler
revisions; build options and compiler versions are captured from each build.
For manual reproduction, check out that revision with its QBE submodule into a
separate directory, configure Release, build target `ada`, then pass its driver
and build directory to the collector. Both compilers must use their own runtimes
and predefined sources; compilation remains outside the timed interval.

[Matched Darwin arm64 results](../../docs/host-validation.md#matched-historical-comparison)
and [full samples](../../docs/host-performance-darwin-arm64.json) cover pre-Phase-2,
end-of-Phase-2 and current builds. Current scalar/image workloads improve, but
block/allocation regressions remain open; the large differences arise after
Phase 2. This is workload-specific evidence, not a phase-wide no-slowdown result.

The subsequent [master activity repair](../../docs/host-validation.md#empty-master-cleanup-repair)
removes empty-scope registry locks. On native Darwin arm64, 21 alternating
trials improve blocks/allocation by about 27% against the preceding compiler;
scalar/image times are unchanged. Full [before/after and historical samples](../../docs/host-performance-master-activities.json)
are retained. Blocks still create an arena-backed aggregate temporary and copy
it to a fixed local; allocation still uses the shared storage registry. Both
remain slower than pre-Phase-2. [Native CI confirmation](../../docs/host-performance-master-activities-ci.json)
now passes all suites, verifies the 16-byte master layout and captures every
revision field. Residual regressions remain on all active CI hosts; separate
runner executions do not provide an isolated before/after speedup for the repair.

The next [aggregate/registry repair](../../docs/host-validation.md#aggregate-temporaries-and-registry-transactions)
uses zeroed stack temporaries for small unmanaged aggregates with known byte
counts, preserving aliasing and exception behavior. Deallocation lookup/unlink
uses one shared critical section, with user callbacks outside it. Native Darwin
arm64 paired runs show about 55% lower block time; allocation is about 3% lower
in the combined pair, which needs longer measurements before a broad claim.
[All staged and historical samples](../../docs/host-performance-aggregate-registry.json)
are retained. [Native CI confirmation](../../docs/host-performance-aggregate-registry-ci.json)
now passes 1,942 tests on each Ubuntu architecture and 1,941 on MSYS2, including
all four new aggregate controls. Blocks remain about 1.55–1.97x historical and
allocation 2.74–5.67x; residual slowdowns remain open.

The subsequent [coallocation/constant-size repair](../../docs/host-validation.md#default-pool-coallocation-and-constant-aggregate-sizes)
combines default-pool objects and registry footers in one cleared heap block and
reuses proven aggregate byte counts while retaining the pre-copy exception check.
Native Darwin arm64 pairs show allocation about 11% faster and blocks about 7.5%
faster than the preceding build. [Full samples and validation](../../docs/host-performance-coallocation.json)
are retained. [Native functional confirmation](../../docs/host-performance-coallocation-ci.json)
passes 1,945 tests on each Ubuntu architecture and 1,944 on MSYS2. Short CI
pairs do not reproduce a uniform speedup across separate runs; Ubuntu amd64
block/allocation ratios are worse this time. Longer runs and batches with
multiple live objects are needed before tuning registry scaling. The immediate
allocate/free workload does not exercise long registry searches.

The [extended schema-3 protocol](../../docs/host-validation.md#extended-and-batched-measurements)
keeps the short sources unchanged, repeats each kernel 128 times per process,
adds a startup control, and measures FIFO/LIFO/permuted batches. Scaling cases
perform 65,536 allocations at 32/512/4,096 live objects; throughput cases perform
4,194,304 allocations at 32 live objects. Pointer tables are heap-backed for the
historical Windows compiler. Every generated source, parameter, sample, order,
dispersion and executable hash is retained; startup is never subtracted.

```sh
python3 -B tests/host/measure.py --ada build/ada/ada --build build \
    --baseline-ada /path/to/before/ada --baseline-build /path/to/before/build \
    --suite extended --repetitions 128 --batch-sizes 32 512 4096 \
    --batch-objects 65536 --throughput-objects 4194304 --runs 11 \
    --work build/host-evidence-extended
```

[Local full reports](../../docs/host-performance-extended-darwin-arm64.json) show
repeated block/allocation ratios of 2.691/4.060 against pre-Phase-2. At 4,096
live objects, current FIFO takes 1.057s versus 0.013892s LIFO for the same total
allocations, consistent with linked-record lookup/unlink costs. The cases also
include array/index work; they are not isolated helper profiles. Generated long
kernels add nested activations, so do not pool their medians with short runs.
All 21 paired cases now validate on all active native CI hosts, with 11 pairs
per case. [Full native reports](../../docs/host-performance-extended-ci.json)
confirm 1,946 tests on each Ubuntu architecture and 1,945 on MSYS2, with no skips.
At 4,096 live objects, current FIFO is about 43–66x LIFO across those hosts.
The long block/allocation gaps remain; the next step is lookup/unlink scaling
while preserving synchronization, ownership precedence and callback behavior.

The subsequent indexed-registry/direct-unlink repair has
[matched local evidence](../../docs/host-validation.md#allocation-registry-indexing-and-direct-unlinking)
and [full reports](../../docs/host-performance-allocation-index.json). At 4,096
live objects FIFO/permuted improve about 75x/61x over the preceding compiler,
while LIFO is about 2.8% slower. Long allocation is about 3.4% slower, and
32-live LIFO throughput about 2.3% slower; long blocks are essentially unchanged.
The optional index activates at 64 live records and falls back on allocation failure.
[Native CI confirmation](../../docs/host-performance-allocation-index-ci.json)
now passes all tests on Ubuntu amd64/arm64 and MSYS2. All 21 cases validate over
11 pairs; 4,096-live FIFO/LIFO ratios are 1.020/0.980/0.966. Historical long
block/allocation gaps remain, and these CI pairs use the pre-Phase-2 baseline,
so cross-run differences do not isolate the repair's cost. Remaining overhead
profiling is the next step.

The next [local runtime profiling study](../../docs/runtime-profiling.md) uses
untimed source-entry counters and seven generated-QBE variants over 11 trials,
with byte-identical copy controls. It selects the empty finalization entry path:
the allocation kernel makes about 17 mostly-empty calls per object, and guarding
those entries reduces diagnostic time about 27%. Master omission reduces blocks
about 34% but is not a valid general optimization. Production builds and this
21-case measurement suite are unchanged; the diagnostic helper is POSIX-only.
[Every diagnostic sample and census](../../docs/host-performance-runtime-profile.json)
is retained. Thirteen protocol controls and both host protocol CTests pass.

The selected [empty finalization entry repair](../../docs/host-validation.md#empty-finalization-entry-path)
now passes the local full suite and sanitizer controls. Its small public entry
avoids TLS and the saved-occurrence frame for empty chains, with the nonempty
algorithm retained in a private non-inlined helper. Matched 21-case/11-pair
production runs reduce long allocation about 20% and 32-live LIFO throughput
about 24%; long blocks are essentially unchanged. All inputs, samples and code
inspection remain in [full local reports](../../docs/host-performance-finalization-entry.json).
[Native confirmation](../../docs/host-performance-finalization-entry-ci.json)
now passes 1,946 tests on each Ubuntu architecture and 1,945 on MSYS2, with all
21 cases and 11 pairs per host validated. Historical long allocation ratios are
4.787/5.454/2.846; block ratios 2.448/2.399/1.974. Allocation ratios are lower
than the preceding CI run, consistent with the local result, but cross-run
comparisons do not isolate the repair. The historical no-slowdown goal remains
open; conservative empty-master proof is the next investigation.

The following [conservative block-master repair](../../docs/lexical-master-elision.md)
uses default-sensitive call effects and preserves lexical cleanup metadata.
Matched 21-case/11-pair production measurements reduce long blocks about 35%;
long allocation and throughput are essentially unchanged. The short 32-live
permuted case is 3.1% slower, while its long throughput control is unchanged.
[Full local evidence](../../docs/host-performance-block-masters.json) retains all
samples, source/build hashes, codegen controls and the unchanged runtime payloads.
Local 1,953 tests, 27 ASan/UBSan and 15 TSan checks pass.
[Native confirmation](../../docs/host-performance-block-masters-ci.json) now passes
1,953 on each Ubuntu architecture and 1,952 on MSYS2. All 21 cases/11 pairs
validate; historical block ratios are 1.908/1.735/1.592, allocation
4.901/5.454/3.185. Runner CPUs changed on amd64/Windows, limiting cross-run
attribution, including the higher Windows allocation ratio. The measurement
protocol/CI matrix are unchanged; historical no-slowdown remains open.

Routine [Build and Test](../../.github/workflows/build-and-test.yml) CI builds and
runs tests without timing collection or a historical-baseline build. The separate
[Build and Profile](../../.github/workflows/build-and-profile.yml) workflow is
manual-only. It builds only the compiler/runtime and profiling probes, then
records the existing extended 21-case/11-pair measurements in `profile-*`
artifacts. Select `baseline_ref` as the pre-batch commit for a same-run comparison;
it defaults to pre-Phase-2 for historical tracking and records the resolved SHA.
Use local profiling between native checkpoints. Mac runners remain commented.

[Remaining helper profiling](../../docs/helper-profiling.md) uses
`profile_runtime.py --suite helpers` for fixed-corpus array/aggregate probes and
single-thread allocation diagnostics. The local 11-trial study selects equivalent
inline scratch zeroing (26% diagnostic block reduction), retaining all initialized
bytes and existing fallbacks in the planned production repair. All 21 ordinary cases remain in a
same-build harness control; no new historical or production speedup is claimed.
Twenty protocol controls and both host protocol CTests pass. Native profiling
remains manual; zero/lock omissions are diagnostic-only.

The [production scratch zeroing repair](../../docs/inline-scratch-zeroing.md)
reduces local long block time 24.7% against the immediately preceding compiler,
using all 21 cases and 11 alternating pairs. Long allocation is 0.9987 of baseline;
throughput ratios are 0.9904–1.0007. Backend/runtime bytes are identical between
builds. [Raw evidence](../../docs/host-performance-inline-zeroing.json) includes
the working-tree patch and full validation results. Native and historical
performance claims remain separate.
