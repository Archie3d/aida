# Performance batch wrap-up

The measured constant-offset and small-copy findings are implemented. Further
profiling is deferred so Phase 8 functional work can proceed. Existing timing
reports remain archived; this batch makes no new production speedup claim and
does not close the historical no-slowdown goal.

Array indexing folds an offset only when the index, bounds and stride are
literals and bounds membership, exact signed widening, nonnegative unpacked
stride and signed 64-bit distance/product are proven. It still evaluates the
prefix/index/stride, publishes the source location and checks pending exception
status. Dynamic, null, packed, invalid and overflowing cases keep the helper;
slice lowering is unchanged. `constantarrayoffsets` checks negative and extreme
64-bit bounds, dynamic and null failures and packed access. Its codegen check
requires both location publication and the exception branch after a folded value.

Static unmanaged aggregate copies up to 32 bytes use inline loads/stores only
when both pointers name known stack allocations. Alignment comes from the actual
QBE allocation, rather than an assumed type alignment. All source bytes are read
before any destination write; aligned smaller chunks cover tails exactly. Unknown
pointers, larger/dynamic sizes and managed/limited paths retain existing helpers.
Zeroing and copy lowering share the same size limit.

`codegen.aggregate_zeroing` now also checks complete copy coverage and read-before-
write ordering. It executes the actual emitted instructions on dirty memory for
same-buffer, forward/backward overlap and disjoint cases, checking every byte and
surrounding canaries. Existing aggregate self-reference, failed component/finalizer
and whole/separate/determinism cases cover destination isolation and commit checks.

The broader performance goal and native checkpoints remain open. Profiling is a
future follow-up, not a gate on the next functional compiler feature.

Validation accompanies the [functional batch](prefixed-views.md#validation-evidence):
1,968 distinct Release checks, 82 selected ASan/UBSan and 43 distinct selected TSan checks
pass. The archived run details distinguish the stale-build rerun from compiler
failures. Existing native timing/validation reports remain unchanged.
