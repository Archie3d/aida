# Host and target validation

AIDA targets the build host and its C toolchain. The supported targets use
64-bit little-endian layouts. The latest native C interoperability evidence
covers all four hosts. [Phase 13 integration audit](interfaces-c-abi.md#integration-audit).

## Native matrix

| Host | Toolchain and test environment |
| --- | --- |
| macOS arm64 | Apple Clang, local validation on Apple silicon |
| Linux amd64 | GCC on `ubuntu-24.04` |
| Linux arm64 | GCC on `ubuntu-24.04-arm` |
| Windows amd64 | MinGW-w64 GCC in MSYS2 UCRT64 on `windows-2025` |

[Build and Test](../.github/workflows/build-and-test.yml) runs the native
Linux and Windows suites on pushes to `master`, pull requests and manual
dispatch. It keeps the source and build paths spaced and publishes JUnit,
logs, ABI facts and the tested revision in `host-*` artifacts. Native test
reports give the status of capability-dependent tests, such as privileged
Linux task-priority checks.

## What the suite checks

- **ABI and execution.** C reports verify scalar widths and signedness,
  alignment, pointer width, endianness, and runtime layouts including array
  descriptors, trace frames and masters. Ada/C fixtures exercise mixed
  register/stack calls, signed and unsigned extremes, writes through Ada
  addresses, and whole-program and separate-unit builds. C `long` follows
  LP64 on macOS/Linux and LLP64 on Windows. Ada `Long_Integer` remains 64-bit.
  [C ABI contract](interfaces-c-abi.md).
- **Installation and lookup.** The relocation test moves a temporary
  installation and compiles from an unrelated directory. It covers spaced
  paths, relative install destinations, `lib` and `lib64` layouts, direct
  and PATH invocation, POSIX symlinks, and the `ADA_INCLUDE_PATH`,
  `ADA_LIBRARY` and `ADA_RUNTIME` overrides. The host C toolchain is still
  required. [Unit source layouts](unit-source-layouts.md).
- **Windows portability.** Tests cover drive letters and native separators,
  `.exe` discovery, canonical source identity through symlinks, raw-QBE
  stack probing, relocated PATH lookup and LF/CRLF handling in ACATS grading.
  [Recorded MSYS2 runs](evidence/host-validation-ci.json).
- **Measurement tooling.** Protocol tests exercise workloads, timeouts,
  failure reporting and paired-run bookkeeping. Timings are collected
  separately through [manual performance validation](performance-wrap-up.md).

## Reproduce

From the repository root:

```sh
cmake -S . -B 'build with spaces' -DCMAKE_BUILD_TYPE=Release
cmake --build 'build with spaces' --parallel 3
ctest --test-dir 'build with spaces' -L host --output-on-failure
ctest --test-dir 'build with spaces' --output-on-failure --parallel 3
```

On MSYS2 run in UCRT64 and configure with `-G 'MinGW Makefiles'`. To exercise
the alternate library layout, configure with `-DCMAKE_INSTALL_LIBDIR=lib64`.

## Historical validation

The initial MSYS2 run exposed eight failures in source alias resolution,
stack-probe registration, artifact directory selection, relocated executable
lookup and ACATS line endings. The native rerun passed all eight and the added
portable harness check. The fixes use Windows file-handle identity, propagate
the stack-probe target, select an explicit artifact directory, resolve the
executable against the test's PATH, and normalize CRLF before grading.

Earlier milestone reports preserve tested revisions, ABI layouts, relocation
results and test totals. Their counts describe the recorded builds.
[Darwin arm64 report](evidence/host-validation-darwin-arm64.json),
[Linux and Windows CI reports](evidence/host-validation-ci.json).
