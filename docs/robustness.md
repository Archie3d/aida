# Robustness validation

Phase 8 establishes four checks: bounded test execution, malformed-source and
mutation coverage, repeatable sanitizer builds, and observation of generated Ada
memory accesses. These checks validate the implemented subset; they do not grade
Ada conformance or prove the absence of crashes and races for every input.

## Running the checks

Python 3.9+, CMake 3.20+ and the existing C/C++ toolchain are required. The presets
use separate build trees, including separate runtime archives and Ada caches:

```sh
cmake --preset release
cmake --build --preset release
ctest --preset release

cmake --preset asan-ubsan
cmake --build --preset asan-ubsan
ctest --preset asan-ubsan

cmake --preset tsan
cmake --build --preset tsan
ctest --preset tsan
```

`AIDA_SANITIZER=none|address-undefined|thread` selects the build configuration.
ASan/UBSan instruments the compiler, driver, C runtime and native test code;
undefined behavior is nonrecovering. TSan also instruments those native targets.
The driver supplies the matching sanitizer link option to its generated
executables. Sanitizer presets select the robustness harness, relevant C runtime
checks and (for ASan/UBSan) the rejection suite. They are validation builds; use a
regular build for installation. Raw foreign-link test scripts outside the
selected sanitizer suites are not a claim of sanitizer coverage.

The generated-access TSan configuration requires Clang on a POSIX host. The
presets were exercised on macOS arm64; other host/target execution retains its
existing Phase 8 owner. No test silently counts an unavailable sanitizer as a
pass. Normal release validation still runs the complete CTest selection.

## Deadlines and process outcomes

Every registered CTest test has a finite `TIMEOUT`. Existing explicit limits are
preserved; `AIDA_TEST_TIMEOUT` supplies a default of 180 seconds to tests without
one. Common Ada execution, diagnostic, IR and determinism runners additionally
limit each compiler command to 60 seconds. Ada programs run for at most 20
seconds. `RunTasking.cmake` preserves explicit `COMMAND_TIMEOUT` overrides for
compilation, with a separate program deadline. These script-level defaults can
be overridden through `AIDA_COMPILE_TIMEOUT` and `AIDA_RUN_TIMEOUT` when invoking
the runner. Nonzero exits, signals and timeout status strings cannot pass as
successful compilation.

The Python robustness runner also limits combined stdout/stderr to one MiB and
records normal exits, crashes, timeouts and output overflow separately. A timed
out process group is killed and reaped on POSIX; Windows uses `taskkill /T`.
A macOS group that disappeared during cleanup is reaped without mistaking that
race for a compiler fault; denial of cleanup for a live group remains a failure.
Process-protocol controls exercise normal success, a diagnosed rejection, a real
abnormal exit, a hang, output overflow and termination of a spawned descendant.
Legacy CMake runners use their deadlines rather than the Python output cap.

## Corpus, mutation and minimized failures

`robustness.malformed_fuzz` runs 10 legal controls, 11 rejection/capacity controls
and 192 byte mutations with seed 819. It invokes the real `adac` pipeline,
including loading, lexing, parsing, semantic analysis and IR emission. The seeds
include scalar initializers, arrays/aggregates, records, functions, packages and
generics; mutations delete, insert, replace and truncate bytes, including invalid
UTF-8 and NUL. Legal controls also exercise moderate nesting and long arithmetic.

A successful compile must produce nonempty IR. A rejection must exit normally
with status 1 and an error diagnostic. Signals, timeouts, excess output,
sanitizer failures and rejection without diagnostics fail. Oversized syntax
controls require their specific capacity diagnostic, so an unrelated rejection
cannot satisfy them. Mutation acceptance is a robustness observation, not an
ACATS legality grade. This is a deterministic mutation campaign, not a
coverage-guided fuzzer.

For longer campaigns:

```sh
python3 -B tests/robustness/fuzz.py --adac build/adac/adac \
    --work /tmp/aida-fuzz --seed 1337 --iterations 2048
```

`observations.json` records the seed, case identity, source SHA-256 and outcome.
A failing input and its captured diagnostics are retained as `<case>.adb` and
`<case>.json`; the current `p.adb` is also available. The campaign is reproducible
with the same seed and Python interpreter. It never executes mutated programs.

The initial campaign exposed long expression/name chains exceeding their time
budget. The parser now diagnoses more than 128 nested guarded parser frames,
128 binary-expression nodes in depth, or 128 name suffixes. Capacity failures
bypass speculative expression-to-aggregate recovery and stop the current parse;
otherwise a depth error could be retried exponentially. Up to 100 errors are
printed, followed by a suppression diagnostic; error counting and rejection
continue. These are documented implementation capacities, not silently ignored
constructs.

The longer campaign found a null-type dereference in an initialized object
declaration after an unknown subtype name. Object analysis now stops after the
failed subtype resolution, preserving its diagnostic. A minimized reproducer
is part of the fixed corpus, independently of mutation seed order.

## Generated Ada access instrumentation

In TSan builds, the QBE adapter applies `tools/tsan_ir.py` before invoking the
unchanged backend. Both driver compilation and raw per-unit test lowering use
this adapter. The adapter's digest includes the pass implementation, so changing
the pass invalidates the driver's object cache. Original SSA and interface
digests remain unchanged. `qbe/` is not modified.

The pass emits LLVM TSan `read_range_pc`/`write_range_pc` callbacks for every
explicit QBE scalar load/store, preserving its byte width, and for both ranges
of a `blit`. Byte ranges cover unaligned and packed accesses too. The supplied
PC identifies the containing generated routine. Unsupported memory instructions
and double instrumentation fail rather than passing through unnoticed. An
independent instruction inventory checks coverage and operand preservation.
The callbacks follow LLVM's [TSan runtime interface](https://github.com/llvm/llvm-project/blob/main/compiler-rt/lib/tsan/rtl/tsan_interface.h).

The C runtime and C tasking helpers are compiled with TSan, so mutexes,
condition variables, thread creation/join, atomics, memory allocation and C
memory operations participate in the same race detector. Atomic operations
remain runtime atomics; the adapter does not turn them into ordinary accesses.
The pass does not add artificial Ada synchronization.

`robustness.sanitized_tasking` runs seven existing programs in whole and separate
compilation: task concurrency/lifecycle and finalization, basic tasks, protected
concurrency, protected entries, delays, shared scalar/atomic operations and task
identity. Clean cases require exact expected output, empty stderr and success.
With TSan it additionally runs a deliberate Ada-to-Ada write race in both modes.
A protected start gate ensures both workers are active before racing. Both must
produce a TSan data-race report and exit 66; an arbitrary crash cannot satisfy
the control. ASan/UBSan runs the 14 clean executions; TSan runs 16 executions.
`coverage.json` records case/mode and instrumented-access counts; `run.json`
retains each execution's result. There are no race suppressions. The negative
control disables external symbolization and abort-on-error to keep the required
report/exit protocol independent of host symbolizer availability.

Instrumentation covers explicit generated memory operations and instrumented C
runtime operations. Backend register spills are private stack mechanics; implicit
foreign ABI copies, arbitrary uninstrumented foreign code and facilities outside
the supported subset are not a claim of complete instrumentation. PCs and SSA
identify access sites; source-level debug mapping and complete Ada call stacks
are not provided by this pass. ASan/UBSan still do not instrument generated QBE
memory accesses. Passing TSan checks establish observed coverage, not proof that
all possible schedules are race-free. See [LLVM's TSan documentation](https://clang.llvm.org/docs/ThreadSanitizer.html)
for detector overhead and platform restrictions.

## Recorded validation

| Check | Result |
| --- | --- |
| Complete regular build CTest run | 1,722/1,722 pass, including 209 ACATS-label checks |
| ASan/UBSan preset selection | 287/287 pass; the final corpus controls also pass with the sanitized compiler |
| TSan preset selection | 14/14 pass, including the 16 whole/separate Ada executions |
| Default corpus | 213 cases: 10 legal, 11 rejection/capacity controls and 192 mutations |
| Longer seed-1337 campaign | 2,048 mutations pass; the subsequently added diagnostic-cap control passes separately |
| Repeated deliberate Ada race | 20/20 detections across the two build modes |
| Deadline inventory | All 1,722 registered regular-build tests have a positive TIMEOUT |
| Diff/source hygiene | `git diff --check` passes; `qbe/` and C runtime sources are unchanged |

The descendant-cleanup protocol controls pass on the tested host. The full
regular suite and both sanitizer selections are distinct checks; the selected
sanitizer results do not imply every regular test was run under sanitizers.
