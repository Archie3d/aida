# Finalization registration scaling

The benchmark retains unrelated integer allocations, then performs 2,000
identical temporary `Unbounded_String` operations. Previously, every registration
searched the complete allocation registry to decide whether its chain belonged
to a heap allocation. This made fixed temporary work linear in the live heap.

The optional allocation index now also indexes the addresses of allocation
finalization chains. Both tables grow together under the existing shared storage
mutex; chain addresses remain stable when deferred allocations publish or change
object addresses. Unlink removes both entries before callbacks or storage release.
Small registries and failed index allocation retain the existing list fallback;
failed growth keeps both previous tables usable. No new runtime ABI or compiler
emission change is required.

Removing a task-local finalization record now stops after successful local
removal, avoiding a second search through unrelated shared records. Heap records
remain shared across synchronized tasks. Finalizers still run outside the storage
mutex with existing exception suspension, ordering and activation semantics.

The private allocation record grows by one pointer. The optional table uses two
pointer buckets per capacity slot instead of one; the enable threshold remains
64 live records. Hash collisions retain bucket walks. Low-memory list fallback,
object-based activation lookup, shared finalization removal and deferred object
publication can still perform linear searches. This change addresses ordinary
temporary registration; it does not claim constant time for every finalization
operation or resolve the broader historical performance goal.

## Reproduction and validation

From the repository root:

```sh
build/ada/ada --force tests/benchmarks/finalizationscaling.adb -o /tmp/finalizationscaling
/tmp/finalizationscaling 0
/tmp/finalizationscaling 1000
/tmp/finalizationscaling 6000
```

Each run must print `Checksum: 12000`. Timing covers only temporary-string work,
excluding retained allocation creation and collection cleanup. Timing is manual;
routine CI checks ownership and cleanup without machine-dependent deadlines.
The [matched samples](evidence/finalization-scaling-measurements.json) compare the same
Release compiler and runtime archive members, replacing only `adart.c.o` with
the pre-fix source for the baseline.

`runtime.array_storage` exercises owner classification below and above the index
threshold, growth, optional enable/growth failures, deferred publication, aliases,
arbitrary removal and local cleanup beside live heap finalizations. It verifies
all managed objects and allocation/index storage are released. The four-worker
`runtime.task_context` control alternates small and indexed live sets while
registering and cleaning up objects concurrently, including failing finalizers.

Local Darwin arm64 Release medians (seven alternating pairs after warmup):

| Retained allocations | Before | After |
| --- | --- | --- |
| 0 | 7.915 ms | 7.867 ms |
| 1,000 | 39.629 ms | 7.896 ms |
| 6,000 | 195.350 ms | 7.939 ms |

Every checksum is 12000. The 6,000-allocation workload is about 24.6 times
faster; fixed-work time remains essentially flat. These are local measurements,
not native-host confirmation or a claim about the historical benchmark suite.

Validation: 50 focused Release checks pass. The full 2,208-check Release run
passes 2,207 initially; `loading.semantic_interfaces` rejects an overlapping
runtime rebuild with `inputs changed during build; retry`, then passes both
its separate recheck and final `--rerun-failed` run with the build stable.
Both expanded runtime CTests pass under ASan/UBSan and TSan. `git diff --check`
passes; QBE and the installed compiler remain unchanged.
