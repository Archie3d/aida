# Remaining offset and copy costs

This local study uses the production compiler after small aggregate scratch
zeroing (`7be7a73f84cac226420f830589d43506b770f839`). It retains that zeroing and
compares checked offset lowering, constant offset folding, a small copy and their
combinations. The probes modify generated benchmark IR only.

## Method and contracts

`profile_runtime.py --suite residual` pins the same reviewed Ada kernels and
requires the complete production two-store zeroing of the 16-byte stack scratch.
Seven block variants and two allocation controls run 11 rotated/reversed trials,
with 128 activations per timed process. Checksums validate silently before timing;
startup is included without subtraction. Original-driver, relinked-control and
control-copy kernel objects must match. The allocation variants are identical
controls, with no locking or cleanup omissions. Census counters run separately
on one activation, including a check that neither kernel creates an Ada task.

The checked-offset probe keeps both signed bounds guards, `Constraint_Error`
and the original pending-exception branch. Injected indices 0 and 5 must match
original failure status, message and trace. Dynamic/null/packed/overflowing shapes
are rejected by the diagnostic rather than treated as valid unchecked offsets.

The constant-fold probe accepts a literal index, or this compiler's exact signed
32-bit literal extension, with literal bounds/stride. It requires valid bounds,
a nonnegative nonpacked stride and signed-range distance/product. Invalid,
nonliteral or unproven cases are rejected. It leaves source-location publication
and the pending-exception branch in place. A future production repair must keep
the existing helper for every unproven case. Folding and inline checks produce
different machine code on this host: the folded block has a 128-byte frame and
four saved general registers, versus 144 bytes and five for the checked probe.
Both include the frame-pointer/link-register pair separately. The measured effects
therefore include register allocation and layout, rather than isolated call cost.

The copy probe is restricted to the pinned, aligned 16-byte stack buffers. It
loads both source words before either destination store, retaining overlap
ordering. It preserves production scratch initialization and the pre-copy
exception barrier. This probe is not a general production memmove replacement.

## Matched results on Darwin arm64

| Block variant | Median (s) | Variant / control |
| --- | ---: | ---: |
| Production control | 0.200776 | 1.0000 |
| Identical-object copy | 0.200402 | 0.9981 |
| Inline checked offset | 0.183845 | 0.9157 |
| Fold proven constant offset | 0.184239 | 0.9176 |
| Inline small copy | 0.187340 | 0.9331 |
| Inline checked offset + copy | 0.152290 | 0.7585 |
| Fold constant offset + copy | 0.145256 | 0.7235 |

Constant folding reduces diagnostic block time **8.2%**, small copy 6.7%, and
folding plus copy 27.7%. The preliminary five-variant study found checked offset
and copy ratios of 0.9302/0.9573 and their combination 0.7808, versus
0.9157/0.9331/0.7585 in the final matrix. Common variants have identical kernel
objects across the studies. Do not pool these separate runs or add individual
gains: helper removal changes alias analysis, register allocation and layout.
Only the final matrix measures the folded-offset variants.

The final allocation control/copy medians are 1.275000s/1.274209s (ratio 0.9994).
The preliminary identical controls differ by about 1.3%, illustrating local timing
variation. All samples, MADs and trial orders are retained. The untimed census
still records 200,000 offset entries per block activation, and the allocation
census matches the preceding study; these are source entries, not CPU percentages.

## Controls retained

The full [21-case production comparison](host-performance-inline-zeroing.json)
from the preceding repair remains the control set: 11 pairs, short/long kernels,
small/LIFO cases, 32/512/4,096-live scaling and 32-live throughput. Its current
compiler/backend/runtime hashes match this study exactly. It is retained as
existing evidence, not a newly timed same-build comparison or historical baseline.
The unchanged production components do not require another full compiler or
sanitizer run for this tooling-only change.

Twenty-three Python protocol checks and both host protocol CTests pass, including
complete production-zero shape checks, retained exception barriers, load-before-
store ordering, literal-extension validation and rejected unsafe fold cases.
The previous 1,957 distinct correctness checks and 59 ASan/UBSan / 20 focused
TSan checks remain the production validation evidence; they were not repeated.

## Selected next repair

Fold only proven-valid constant unpacked array offsets. Preserve evaluation of
the prefix, index and stride, access/null checks, source-location publication
and the original pending-exception branch even when no helper call remains.
Retain the runtime helper if literal/widening, bounds or overflow proof fails;
keep dynamic, packed and slice behavior on existing paths. In particular, do not
fold invalid bounds/index pairs into a success or replace `Storage_Error` on
unsupported strides/products with wrapping arithmetic.

Start at array indexing, keeping the existing signed 32-bit widening semantics
for narrow literal indices and signed 64-bit limits for wide operands. This is a
smaller production change than general variable-index lowering. Validate the
actual repair with correctness/sanitizer controls and a matched 21-case production
comparison before attributing these diagnostic gains to the compiler.

[Complete evidence](host-performance-residual-helpers.json) preserves both full
studies, the smoke run, collector sources, failure controls, all hashes and the
retained 21-case report. Native checkpoints remain deferred while work continues
locally. Historical no-slowdown remains open.

## Reproduce

```sh
python3 -B tests/host/profile_runtime.py --build build --suite residual \
    --work build/host-performance/residual-helpers/manual --runs 11 --repetitions 128
ctest --test-dir build --output-on-failure \
    -R '^host\.(measurement_protocol|harness_protocol)$'
```

Use the recorded post-zeroing Release build at `7be7a73f84cac226420f830589d43506b770f839`
without competing validation load. The production findings are now
[implemented](performance-wrap-up.md); later IR intentionally fails the fixed
probe shapes. Further profiling is deferred while functional work continues.
The original `helpers` suite continues to require its older pre-zeroing IR shapes;
`cleanup` remains the default. Native profiling is a manual milestone checkpoint.
