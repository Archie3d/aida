# Inline aggregate scratch zeroing

Small static aggregate scratch buffers now use aligned zero stores instead of a
`memset` call. The limit is 32 bytes, with at most six stores. Stores cover exactly
the allocated bytes, including any padding; this does not remove initialization.
A null aggregate still clears its one-byte scratch allocation. Scratch remains
separate from the destination, and component evaluation order, source-location
publication, range/coverage checks and the pre-copy exception barrier are retained.

The change applies only to the existing unmanaged, nonlimited static scratch
path. Sizes above 32 bytes retain `memset`; dynamic/large/managed cases retain
their existing allocation and initialization paths. The store widths use the
four/eight-byte alignment guaranteed by the selected QBE stack allocation and
smaller aligned stores for tails. Other zeroing and aggregate copies are unchanged.

## Validation

`aggregatezeroing` covers null, 1/2/3/4/5/7/8/9/15/16/17/31/32/33-byte,
large and dynamic arrays, in whole-unit, separate-unit and deterministic modes.
`codegen.aggregate_zeroing` checks exact byte coverage and alignment before
component evaluation, the source-location store and retained fallbacks. It also
executes the compiler's actual zero instructions on dirty scratch memory with
canaries on both sides, independently of later aggregate value stores. Existing
aggregate self-reference and component/finalizer exception tests cover destination
isolation and the commit barrier; box/managed tests retain their previous paths.

The Release run passed 1,956/1,957 checks; the only failure was an obsolete
command line for the newly extended codegen test, loaded before registration
was regenerated. The corrected four-test boundary rerun passes, giving 1,957
distinct passing correctness checks. All 59 focused ASan/UBSan checks and 20
distinct focused TSan checks pass, including the final boundary reruns. Generated
Ada memory accesses receive TSan instrumentation; ASan/UBSan instrument the native
compiler/runtime, while the standalone dirty-memory/canary control observes the
actual generated stores directly.

## Local performance comparison

The before build was compiled from `36ce98fbbb8ea5c2324ba712fc84a0936dd6fdb1`,
with the same Release toolchain as the after build. Both drivers use byte-identical
QBE executables and runtime archives, pinned after building the baseline compiler.
The comparison uses the
unchanged 21-case suite and 11 alternating pairs, with all checksums validated,
128 long-kernel activations, small/LIFO controls, 32/512/4,096-live scaling and
32-live throughput. Startup remains included. Timing runs separately from tests
and sanitizer builds. The evidence records source changes as well as executable,
compiler, backend and runtime hashes; the after build is a working-tree build.

| Case | Before (s) | After (s) | After / before |
| --- | ---: | ---: | ---: |
| Long block | 0.266976 | 0.200988 | 0.7528 |
| Long allocation | 1.301090 | 1.299370 | 0.9987 |
| Long task-context control | 1.280718 | 1.281052 | 1.0003 |
| Long image control | 2.574432 | 2.586354 | 1.0046 |
| LIFO throughput, 32 live | 0.482538 | 0.477883 | 0.9904 |

Long block time falls **24.7%**, consistent with the earlier equivalent-zero probe.
Its median absolute deviations are 0.000827s before and 0.000693s after. Short
block time falls 5.6%, with startup still included; short controls remain noisy.
Scaling controls range from 0.9919 to 1.0185, and throughput controls from 0.9904
to 1.0007. This change does not address allocation overhead or establish the
historical no-slowdown goal.

[Complete evidence](host-performance-inline-zeroing.json) preserves every sample,
execution order, dispersion, component hash, the production patch, test sources
and validation outcomes. The earlier [helper study](helper-profiling.md) measured
the opportunity using fixed QBE probes; the table above measures the production
repair. Native correctness CI remains routine; native profiling is a manual
milestone checkpoint. Historical no-slowdown remains open.
