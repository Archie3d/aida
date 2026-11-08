# Aggregate helpers and allocation overhead

The next repair is to inline zeroing for small, known-size aggregate scratch
storage while preserving every initialized byte. A fixed benchmark probe reduces
long block time about 26% with two zero stores, versus 29% from omitting zeroing
altogether. Preserving zeroing captures most of that opportunity without needing
a redundant-initialization proof. This is not yet a production repair.

## Controlled local experiment

`profile_runtime.py --suite helpers` adds two fixed-corpus diagnostic matrices:
eight block variants and seven allocation variants, each measured over 11
rotated/reversed trials with 128 kernel activations. Every executable first
validates the original checksums silently. Production compiler/runtime files are
unchanged. The existing `cleanup` suite remains the default and passes a small
compatibility smoke run.

The helper suite requires the reviewed source hashes of `blockcalls` and
`allocationcalls`. It rejects changed input or unexpected QBE shapes rather than
silently applying unsafe omissions to another program. Every timed variant uses
the same object/link ordering, and original-driver, rebuilt-control and copy
kernel objects must be byte-identical. Runtime copies use the target's PIC mode;
the rebuilt lock object must also match the original archive member byte-for-byte.
Derived archives, sources and objects stay in the diagnostic work directory.

The block probes isolate checked constant-bound array-offset lowering, equivalent
inline zeroing, omission of the fully overwritten 16-byte scratch zero, an inline
16-byte copy, and their combinations. The offset probe retains both signed bounds checks and raises
`Constraint_Error` on failure. Injected indices 0 and 5 produce identical failure
status, message and trace with the original helper and the inline probe.
Unsupported dynamic/packed/overflowing offset shapes fail the diagnostic. The
copy reads both source words before either write, retaining overlap ordering;
the corpus uses aligned stack buffers. Zero omission is justified only for this
pinned four-element scalar aggregate, whose normal path writes all 16 bytes.
The equivalent inline-zero variant instead retains both 64-bit zero stores. A
separate QBE control prefills both words with all-one bits, applies either memset
or the replacement and reads all 16 bytes; both executables return zero silently.
This checks zero preservation independently of the aggregate checksum.

The allocation probes retain nonempty finalization/rewind calls behind empty
checks. A separate single-thread diagnostic omits only the environment storage
mutex's lock/unlock; other mutex operations remain. This is not a production
synchronization change. The fixed corpus creates no Ada tasks; the untimed census
checks that property. Shared collections, reentrant pools and task lifetimes still
require the production locking contract.

## Matched helper results on Darwin arm64

| Block variant | Median (s) | Variant / control |
| --- | ---: | ---: |
| Control | 0.266853 | 1.0000 |
| Identical-object copy | 0.268172 | 1.0049 |
| Inline checked offset | 0.247283 | 0.9267 |
| Inline equivalent scratch zero | 0.197151 | 0.7388 |
| Omit scratch zero | 0.189986 | 0.7120 |
| Inline 16-byte copy | 0.214277 | 0.8030 |
| Zero omission + inline copy | 0.183648 | 0.6882 |
| Offset + zero omission + inline copy | 0.143941 | 0.5394 |

| Allocation variant | Median (s) | Variant / control |
| --- | ---: | ---: |
| Control | 1.273176 | 1.0000 |
| Identical-object copy | 1.274348 | 1.0009 |
| Rebuilt identical lock object | 1.272929 | 0.9998 |
| Omit storage locking, fixed single-thread corpus | 1.071777 | 0.8418 |
| Guard empty finalization | 1.126443 | 0.8848 |
| Guard empty arena rewind | 1.233545 | 0.9689 |
| Guard both cleanup calls | 1.085233 | 0.8524 |

These whole-program effects include call boundaries, register allocation,
alias-analysis and layout changes. They are not sampled CPU percentages, and
combined gains are not additive. The largest individual block probe is the
scratch zero omission (about 29%), but equivalent inline zeroing recovers about
26%; the copy and checked offset recover about 20% and 7%. Offset, zero omission
and copy together reduce diagnostic time about 46%, without establishing that
general production transformations would achieve the same gain.

Allocation still has meaningful empty-call and uncontended-lock costs. Guarding
finalization reduces this diagnostic about 12%; the storage-lock experiment and
combined cleanup guards reduce it about 16% and 15%, respectively. This does not
justify deleting locks or assuming every real finalization chain is empty.

## Untimed entry census

Counters run separately from timing on **one** kernel activation, not the timed
128. They count source entries, including inlined/runtime-internal entries, and
perturb execution; they are not counts of machine call instructions.

The block census records 200,000 array-offset entries, matching its iteration
count. The allocation census records 100,000 allocations, registrations, lookups
and deallocations; 400,008 mutex lock/unlock pairs; 1,700,009 finalization entries
(1,700,008 empty); and 500,004 arena rewinds (500,003 empty). Task-context source
entries are 300,021, down from the older pre-fast-path census's roughly 20 per
object to roughly three here. The master activation/await/discard predicate now
checks the actual dependent-task bit, so registered-but-task-free collections
are correctly classified as doing no dependent-task work. Master leave still
uses the stronger zero-activity predicate because it also reclaims registrations
and accessibility state.

## Existing-suite control and limits

The unchanged **21-case/11-pair** suite was also run with the identical compiler
and runtime on both sides, retaining startup, short/long kernels, 32/512/4,096-live
FIFO/LIFO/permuted scaling and 32-live throughput. This is a harness control,
not a historical or before/after repair comparison. All programs validate; ratios
range from 0.974 to 1.026. Long block/allocation/LIFO-throughput ratios are
0.9982/0.9961/1.0005. Startup is included without subtraction in both studies.
The production component hashes match the preceding block-elision build.

Twenty Python protocol controls and both host protocol CTests pass, including
corpus pinning, offset failure checks, copy ordering, shape rejection and keeping
all other mutex operations. Full compiler and sanitizer suites were not repeated
for this diagnostic-tool-only change. No GitHub profiling run is needed for this
local diagnosis; native profiling remains a manual milestone checkpoint.

[Complete evidence](host-performance-helper-profile.json) retains every trial,
order, dispersion, input/build/object/executable hash, census predicate, fault
outcome, independent zero-preservation control and the compatibility smoke
report. The initial seven-block-variant study is retained separately as exploratory
evidence, including its collector sources; the tables use the final eight-variant
study with equivalent inline zeroing. Do not pool the one-activation smoke
or census with the 128-activation timed helper study.

## Selected production repair

Inline zeroing for small, statically sized and suitably aligned compiler-owned
aggregate scratch. Preserve all zero bits, including padding/default storage;
do not infer that subsequent aggregate stores make initialization redundant.
Bound generated code size and retain memset or existing runtime allocation for
dynamic, large, unsupported or insufficiently aligned cases. Keep source-location
publication, separate scratch storage, component evaluation order, range/coverage
checks and the pre-copy exception barrier, including failures from component
temporaries/finalizers.

Validate the actual repair with the full correctness/sanitizer controls and a
matched before/after 21-case production comparison. Keep the other helper and
allocation opportunities separate. Historical no-slowdown remains open.

## Reproduce

Use a matching Release build without concurrent validation load:

```sh
python3 -B tests/host/profile_runtime.py --build build --suite helpers \
    --work build/host-performance/helper-profile/manual --runs 11 --repetitions 128
ctest --test-dir build --output-on-failure \
    -R '^host\.(measurement_protocol|harness_protocol)$'
```

The diagnostic helper supports POSIX hosts. The manual GitHub profiling workflow
continues to use the unmodified standard measurement suite on all active hosts.
