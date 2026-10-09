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
