# Remaining block and allocation overhead

The next repair is a small empty-chain entry path for `__ada_finalize_to`, with
nonempty finalization isolated from its large stack frame and TLS acquisition.
The diagnostic finalization guard reduces the local long allocation workload by
about 27%. This selects a target; it is not a measured production repair.

## Method and controls

Native macOS `sample` could not attach to either benchmark in this environment.
The fallback uses two independent forms of evidence, without elevated access:

- An **untimed source-entry census** inserts counters into copied runtime
  sources and a copied archive. It includes runtime-internal and inlined source
  entries. Counter overhead changes execution; counts are not sampled CPU
  percentages or counts of machine call instructions.
- A **timed generated-QBE experiment** changes copies of the two fixed benchmark
  units, linked with the original runtime. It compares seven variants over 11
  trials, rotating and reversing order. Each executable validates silently once
  before timing, and retains the kernel's checksums. One unmodified-copy variant
  must produce a byte-identical kernel object before measurement proceeds.

Both use the unchanged block/allocation kernels wrapped in 128 activations.
The timed experiments include process startup without subtraction. Census runs
are separate from timed runs. No production sources, archive or QBE files are
modified. Master omission is restricted to these entry-free diagnostic kernels;
it changes general Ada semantics and is not an optimizer available to users.
Cleanup guards retain nonempty runtime calls and all exception branches.

The helper supports POSIX hosts. It rejects Windows before writing diagnostics:
Windows assembly needs the driver's stack-probe preparation. The normal native
Windows 21-case measurement suite remains unchanged.

## Observations on Darwin arm64

Final matched medians, in seconds, from the verified study:

| Diagnostic variant | Blocks | Blocks / control | Allocation | Allocation / control |
| --- | ---: | ---: | ---: | ---: |
| Original | 0.398223 | 1.0000 | 1.598850 | 1.0000 |
| Unmodified object copy | 0.398206 | 1.0000 | 1.600387 | 1.0010 |
| Omit lexical-master hooks | 0.261176 | 0.6559 | 1.428995 | 0.8938 |
| Guard empty finalization | 0.398170 | 0.9999 | 1.171590 | 0.7328 |
| Guard empty arena rewind | 0.398190 | 0.9999 | 1.539858 | 0.9631 |
| Guard both cleanup hooks | 0.398423 | 1.0005 | 1.126808 | 0.7048 |
| Master omission + cleanup guards | 0.261254 | 0.6560 | 0.983903 | 0.6154 |

The exploratory study gives similar ratios, and the two censuses agree exactly.
The allocation census includes 12,800,000 allocations/deallocations:

- 217,601,025 finalization entries; 217,600,897 have owner equal to checkpoint.
  That is approximately 17 mostly-empty entries per allocation, with 128 nonempty
  calls for collection cleanup across the 128 kernel activations.
- 64,000,385 arena rewinds; 64,000,257 are empty.
- 256,002,188 task-context source entries, approximately 20 per allocation.
- 51,200,897 mutex lock/unlock pairs, approximately four per allocation. The
  experiment does not remove these locks or attribute a time share to them.

The block census has 25,600,000 array-offset entries and 25,600,130 master
enter/activate/leave entries each. It counts 51,200,260 awaits, including the await
inside leave. Every recorded block activation, await and leave sees zero master
activity. Omitting this hook family reduces blocks about 34%, but preserves no
proof for arbitrary callers, accessibility identities or dependent task ownership.

Matching Apple clang Release assembly shows `__ada_finalize_to` saving registers
and reserving a total of 656 stack bytes, setting up a stack guard and resolving
TLS before comparing owner with checkpoint. Thus returning immediately in C
source still incurs a heavy entry path on this build. Other toolchains may emit
different code; the census alone cannot establish their instruction costs.

The diagnostic guards skip function entry as well as work inside the callee.
Their gain includes code-layout/register-allocation effects and does not predict
an exact gain from a runtime wrapper. Block timing is unaffected by cleanup guards
because that kernel has no such generated cleanup calls. Historical performance
gaps and the no-slowdown goal remain open.

## Selected repair and validation

Make the public finalization entry check owner/checkpoint before TLS and move
nonempty work behind a small entry path. Verify the actual generated empty-path
code rather than relying only on source reordering. Retain exception/message/
trace restoration, balanced abort deferral, ordering and reentrant/failing
finalizers. The nonempty algorithm and generated-code ABI should remain intact.

Measure matching before/after production builds with the existing **21-case**
suite: original/long kernels, startup, small-registry/LIFO throughput and
32/512/4,096-live scaling cases. Run the correctness and sanitizer controls,
then confirm on native CI. This diagnostic study does not replace those checks.
Conservative proof of when lexical masters may be omitted is a separate target.

[Complete reports](host-performance-runtime-profile.json) retain both studies,
sampling failures, the assembly entry excerpt, all samples, input/build/object/
executable hashes, census predicates, order and dispersion. Thirteen Python
protocol controls and both host protocol CTests pass; production compiler/runtime
hashes match the preceding registry build. The full compiler and sanitizer suites
were not repeated for this diagnostic-tool-only change.

## Reproduce

Use a matching Release build without concurrent validation load:

```sh
python3 -B tests/host/profile_runtime.py --build build \
    --work build/host-performance/runtime-profile/manual --runs 11 --repetitions 128
ctest --test-dir build --output-on-failure \
    -R '^host\.(measurement_protocol|harness_protocol)$'
```

The helper requires the build's C compiler, archiver and QBE, and rejects work
areas that could overwrite sources or compiler/runtime build products. It writes
only diagnostic copies. There is no arbitrary-source option. Call-signature and definition checks fail
on unexpected runtime/QBE shapes rather than silently changing the experiment.

The selected entry-path repair now has
[local correctness, assembly and matched production measurements](host-validation.md#empty-finalization-entry-path).
It reduces long allocation about 20% and 32-live LIFO throughput about 24%; the
benchmark-only 27% guard result remains separate. Native CI confirmation is pending.
