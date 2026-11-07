# MSYS2 CI failure triage

The supplied 2026-11-07 artifacts show **1,937/1,937 PASS** on Ubuntu amd64
and arm64. MSYS2 UCRT64 reports **1,928/1,936 PASS** with eight failures.
Its native threading, LLP64 layout, mixed C/Ada register/stack calls and Windows
host-library checks passed. [Pinned result summary](host-validation-ci.json).
These counts describe the supplied run, before the repairs below.

| Failed tests | Cause | Repair |
| --- | --- | --- |
| loading.sources, loading.completions | Native file reading follows the source symlinks, but MinGW filesystem canonicalization retains the alias identity; the loader parses one source twice and diagnoses duplicate library parts | Resolve an opened Windows file's final normalized path through GetFinalPathNameByHandleW; keep original source spellings for diagnostics/manifests and filesystem fallback for failed opens |
| runtime.discreteranges_modes, runtime.accesssubtypes_modes, runtime.genericchildren_modes | Three registrations omitted STACK_PROBE for separate raw-QBE assembly | Pass the existing Windows stack-probe target, matching the other modes; all 309 RunTasking registrations were checked |
| loading.system_spec | The test selected systemsource.exe as its output but looked for the cache under systemsource.adaobj | Supply an explicit artifact directory with -D; reuse it while checking dependency invalidation |
| host.install_relocation | Python's Windows CreateProcess lookup ignores PATH supplied in env when searching for a bare executable | Resolve the executable with shutil.which against the test PATH; pass it as executable while retaining bare argv[0] so the driver's own PATH-based installation lookup is still exercised |
| acats.support_protocol | The binary-output subprocess adapter retains CRLF, while the protocol test required literal LF | Normalize CRLF for comparison, retaining exact REPORT identity/completion, empty stderr and normal exit requirements; expected negative controls require exit 1, rejecting Windows crash statuses |

The Windows resolver uses a dynamically sized UTF-16 buffer and closes the
file handle on every path. The [Windows API contract](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfinalpathnamebyhandlew)
provides the target path rather than the opened symbolic-link name.
The Python change follows the documented
[Windows subprocess search behavior](https://docs.python.org/3/library/subprocess.html#subprocess.Popen).
No test was excluded or converted to an expected failure.

`host.harness_protocol` adds seven portable regressions for CRLF/LF completion,
partial/wrong/failed REPORT output, exit codes, stderr, crashes, lookup against
the replacement PATH and preservation of bare argv[0]. Existing source-alias
loading tests continue to exercise deduplication and requested-main selection.
The copied-file rejection control remains distinct from aliases of one source.

Local validation passes all 1,938 tests on Darwin arm64, plus the nine focused
affected/harness checks in ASan/UBSan. The portable protocol test contains seven
regressions. Native MSYS2 confirmation remains pending;
macOS execution does not exercise the Windows file-handle resolver. Rebuild and
run the full MSYS2 CI job with these changes. A focused check is also available:

```sh
ctest --test-dir 'build with spaces' --output-on-failure \
  -R '^(loading\.(sources|completions|system_spec)|runtime\.(discreteranges|accesssubtypes|genericchildren)_modes|host\.(install_relocation|harness_protocol)|acats\.support_protocol)$'
```

The uploaded Ubuntu timing records contain current-only samples. They are
host execution evidence, not matched pre/post-Phase-2 performance comparisons.
Windows timing collection did not run because its preceding CTest step failed.
