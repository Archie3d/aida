# Conservative lexical-block master elision

Plain lexical blocks can omit their master record and lifecycle hooks when their
emitted operations cannot observe or publish that master. Function, task, library
and collection master policies remain unchanged. This is narrower than the
benchmark-only master omission used in the earlier profiling study.

## Proof boundary

`emitCallInstruction` defaults to `MasterEffects::MayObserve`. Source calls,
foreign and indirect calls, allocators, accessibility queries, tag ownership,
controlled initialization/finalization, tasking, protected actions and unclassified
runtime operations all retain the enclosing block masters. A source import named
like a known runtime helper still uses this default; no purity is inferred from
QBE symbol text, function names or exception effects.

A small set of emission sites explicitly uses `Independent`: plain memory
zero/copy, array-offset checking, compiler-owned plain arena rewind and basic
exception raising. The audited runtime operations neither read/publish master
ancestry nor invoke Ada callbacks. Array rewind frees plain arena storage;
controlled finalization is a separate, master-observing call. Other sites remain
conservative, including potentially empty finalization calls. Changing an audited
helper's runtime contract requires reviewing its classification.

The candidate enter/activate/await/leave bookkeeping uses a separate `ScopeHook`
classification. These hooks are not claimed to be generally independent: they are
the operations whose necessity is being proved for their owning block.

For each block the emitter saves the function's monotonic count of potentially
observing calls, then emits all declarations, branches, nested blocks and exits.
It can remove the candidate master only if the count does not change. The count
is separate from the lexical block stack, so copying/resizing that stack while
emitting goto, return or exceptional cleanup cannot roll back the evidence.
An observing operation in a nested block also retains every enclosing candidate
whose emission interval contains it. Observing calls before or after a pure
inner block do not force that already-closed block to retain its own master.

For a successful proof, no operation can read the block's identity, assign it an
accessibility token, register dependent tasks or attach collection/tag ownership
to it. Its address occurs only in the emitter's lifecycle bookkeeping and does
not escape. Its activation/await have no dependent work; leave has no registration
or accessibility cleanup. Removing that transient entry preserves the effective
ancestry seen by every supported observing operation outside the block.
Previously published parent links cannot acquire the unobserved new child.
Runtime context binding remains per-thread; unsynchronized foreign manipulation
of another running task's context is outside that contract.

## Control-flow and storage preservation

The emitter keeps the full lexical block stack, transfer depths and storage
checkpoints while generating code. It does not change exception checks, source
trace locations, handlers, ordinary/temporary cleanup or their order. It records
the proven master and filters only its exact record allocation and lifecycle-call
lines after activation-return offsets have been expanded. Every normal and
unwind copy of those hooks is removed together. The matching uses generated
record identities, not a textual inference of call effects.

A return or nonlocal transfer thus still follows the same storage/exception
cleanup edges. Any callback-capable finalization makes the proof fail and retains
the master. Parent cleanup runs after the inner lexical boundary in both cases;
no wait is moved across finalization. A block containing an unknown call keeps
its original current-master ancestry, even if a pure child can be omitted.

## Controls

- `emptymasterblocks` covers nested scalar/array blocks, goto, early returns and
  exception propagation to a surrounding handler. No lexical-block hooks remain;
  the function master policy is retained.
- `blockmastercalls` retains both blocks containing source calls, including an
  imported C function named `__ada_array_offset`. Its pure inner block is omitted.
- `codegen.block_masters` checks those positive/negative results and retains
  lexical masters in managed-cleanup, tasking and anonymous-access fixtures.
- Both new programs run normally, deterministically and in whole/per-unit modes,
  including the existing parallel compilation control. They also run correctly
  with the preserved pre-change compiler.

The full compiler/runtime suites, sanitizer controls and unchanged 21-case
performance suite validate this repair. Matched measurements and native CI status
are recorded in [host validation](host-validation.md). No general interprocedural
purity, function-master elision or task-lifetime shortcut is assumed.

Local validation passes 1,953 Release tests, 27 ASan/UBSan and 15 TSan focused
checks. The matched 21-case/11-pair suite reduces long block time about 35%, with
long allocation essentially unchanged. [Complete reports](host-performance-block-masters.json)
retain every sample and the unchanged-code controls.
[Native confirmation](host-performance-block-masters-ci.json) now passes 1,953
tests on each Ubuntu architecture and 1,952 on MSYS2, including all seven new
registrations and 21 timing cases over 11 pairs. Historical block ratios are
1.908/1.735/1.592; CPU changes limit cross-run speedup attribution. The historical
no-slowdown goal remains open.
