# Performance validation and evidence

The historical goal of matching or improving pre-Phase-2 performance across
the benchmark suite remains open. Some measured workloads improved, while
block and allocation workloads still showed slowdowns against that baseline.
Phase 14 tracks matched production measurements for the offset and copy
changes, plus native milestone checks. [Roadmap](../TODO.md#phase-14---deferred-hostperformance-follow-up).

## Reproduce a matched comparison

Run the candidate and baseline on the same host and toolchain, with matching
Release flags. Keep each compiler with its own frontend, QBE, runtime and
predefined sources. Build and test before collecting timings. For milestone
comparisons use the pre-Phase-2 revision
`3d2aac5980f57fb68524a15b154f65cf0882bdc2`. To measure a specific change,
use its immediate baseline.

```sh
cmake --build build --target ada context_access_benchmark
python3 -B tests/host/measure.py \
    --ada build/ada/ada --build build --revision "$candidate_revision" \
    --baseline-ada /path/to/baseline/ada/ada --baseline-build /path/to/baseline \
    --baseline-revision "$baseline_revision" \
    --context-probe build/tests/context_access_benchmark \
    --suite extended --runs 11 --work build/host-evidence
```

Set the revision variables to the commits used for the builds. In MSYS2 use
native Python and the `.exe` executable names. Without the baseline arguments,
the collector records current-only measurements. The default `short` suite
runs the four original kernels. [Collector](../tests/host/measure.py).

The extended suite has 21 cases. Four short kernels cover scalar calls,
image results, block scopes and allocation. A startup control and four
128-repetition kernels separate short-process overhead from longer workloads.
Nine cases measure FIFO, LIFO and permuted release at live sets of 32, 512 and
4,096 out of 65,536 allocations. Three throughput cases allocate 4,194,304
objects at a live set of 32. Workloads validate checksums before timings are
accepted. Compilation is outside the timed interval. Wall time includes
process startup. Reports preserve samples, pair order, build flags, revisions
and component hashes.

[Build and Profile](../.github/workflows/build-and-profile.yml) collects the
extended suite on Linux amd64/arm64 and Windows amd64 on manual dispatch.
Set `baseline_ref` to choose a different baseline. It publishes `profile-*`
artifacts with ABI and timing JSON. Normal CTest checks correctness and
collector behavior with no performance thresholds.
[Routine host validation](host-validation.md).

## Recorded milestones

Each row describes the measured builds for that milestone. Native CI reports
sometimes come from separate runner instances. Use a same-run pair when
attributing a speed difference to one change.

| Change or study | Result and scope | Raw evidence |
| --- | --- | --- |
| Early pre-/post-Phase-2 comparisons | On Darwin arm64, scalar calls and images improved while blocks and allocation slowed. Native Linux and Windows also showed residual block/allocation costs. The earliest Windows report lacks its candidate revision field. | [Darwin](evidence/host-performance-darwin-arm64.json), [native CI](evidence/host-performance-ci.json) |
| Empty masters | Activity flags avoid registry work for empty masters. The native ABI probe confirms the resulting 16-byte master layout. Matched Darwin runs reduced block and allocation time about 27%. | [Darwin](evidence/host-performance-master-activities.json), [native CI](evidence/host-performance-master-activities-ci.json) |
| Aggregate temporaries and registry transactions | Eligible unmanaged aggregate scratch uses stack storage. Lookup and unlink share a critical section, with callbacks outside it. Matched Darwin block time fell about 55%. | [Darwin](evidence/host-performance-aggregate-registry.json), [native CI](evidence/host-performance-aggregate-registry-ci.json) |
| Default-pool coallocation and constant sizes | Coallocated heap objects and registry records reduce allocator work. Known aggregate byte counts are reused. Matched Darwin allocation time fell about 11%. | [Darwin](evidence/host-performance-coallocation.json), [native CI](evidence/host-performance-coallocation-ci.json) |
| Extended protocol | Longer workloads and batches exposed non-LIFO scaling as the live set grew across the supported hosts. | [Darwin](evidence/host-performance-extended-darwin-arm64.json), [native CI](evidence/host-performance-extended-ci.json) |
| Indexed allocation registry | Address indexing and direct unlink reduced the 4,096-live Darwin FIFO and permuted times about 75x and 61x. The LIFO control stayed close to its earlier time. | [Darwin](evidence/host-performance-allocation-index.json), [native CI](evidence/host-performance-allocation-index-ci.json) |
| Empty finalization entry | Fast paths avoid shared lookup for empty registrations and preserve cleanup semantics. [Runtime profiling](runtime-profiling.md). | [Darwin](evidence/host-performance-finalization-entry.json), [native CI](evidence/host-performance-finalization-entry-ci.json) |
| Lexical block masters | Omit the record and hooks only when block effects prove the master unobservable. Matched Darwin long-block time fell about 35%. [Elision contract](lexical-master-elision.md). | [Darwin](evidence/host-performance-block-masters.json), [native CI](evidence/host-performance-block-masters-ci.json) |
| Helper and scratch investigations | Diagnostic probes isolated runtime entry, aggregate zeroing, offset and copy costs. Inline scratch zeroing reduced matched Darwin production long-block time 24.7%. | [Runtime profile](evidence/host-performance-runtime-profile.json), [helper profile](evidence/host-performance-helper-profile.json), [zeroing](evidence/host-performance-inline-zeroing.json), [residual probes](evidence/host-performance-residual-helpers.json) |
| Temporary finalization | Indexing heap-chain ownership removed a live-set cost in a fixed temporary-string workload. Local measurements at 6,000 retained allocations fell from 195.350 ms to 7.939 ms. [Reproduction and runtime controls](finalization-scaling.md). | [Darwin](evidence/finalization-scaling-measurements.json) |

See the [benchmark fixture guide](../tests/benchmarks/README.md) for the
task-context ABI, result buffers, locking and arithmetic probes.

## Constant offsets and bounded stack copies

Array indexing folds an unpacked offset when the literal index, bounds and
stride prove membership, nonnegative stride and signed 64-bit distance and
product after exact widening. The emitter retains expression evaluation,
source location and pending-exception checks. Dynamic, packed, null, invalid
and overflowing cases use the checked helper. `constantarrayoffsets` covers
extreme bounds and the location and exception branches.

Static unmanaged copies of up to 32 bytes use inline loads and stores when both
pointers refer to known stack allocations. Allocation alignment determines
chunk sizes. All source bytes are read before destination writes. Unknown,
larger, dynamic, managed and limited copies keep their existing paths. The
`codegen.aggregate_zeroing` test executes emitted instructions over disjoint,
overlapping and identical buffers and checks all bytes and surrounding canaries.

The functional batch passed 1,968 distinct Release checks, 82 selected
ASan/UBSan checks and 43 distinct selected TSan checks. The matched diagnostic
probes describe potential gains for the selected shapes. Matched measurements
on the final production offset and copy paths remain pending.
[Functional validation](prefixed-views.md#historical-validation-evidence),
[diagnostic study](residual-helper-profiling.md).
