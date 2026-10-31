# Ada regression fixtures

Fixtures are grouped by their primary feature. Each test keeps its source,
expected output/diagnostics (`.expected`), optional stderr, and supporting units
together. Tests crossing feature boundaries live with the dependency family
that supplies their shared units; for example, controlled container lifetime
cases live in `controlled/`.

| Folder | Coverage |
| --- | --- |
| `access/` | Access types, accessibility, allocators, callbacks and collections |
| `arrays/` | Bounds, descriptors, aggregates, slices and concatenation |
| `basics/` | Minimal compiler smoke test |
| `containers/` | Predefined container APIs, cursors, sorting and storage |
| `control_flow/` | Branches, loops, case statements and goto |
| `controlled/` | Controlled ownership, adjustment, finalization and cleanup |
| `driver/` | Linking, incremental builds, command-line behavior and host ABI |
| `elaboration/` | Elaboration order, categorization, pragmas and check policies |
| `exceptions/` | Handlers, occurrences, propagation and exception-library APIs |
| `generics/` | Formal types/objects/packages/subprograms, contracts and instances |
| `interactions/` | Combined language-feature cases |
| `io/` | Narrow/wide text, numeric, sequential, direct and stream I/O |
| `library/` | Calendar, directories, environment and numeric library APIs |
| `numerics/` | Integer, real, fixed, decimal and modular arithmetic |
| `representation/` | Size, alignment, storage and representation clauses |
| `strings/` | Character/string operations, Unicode source and wide foundations |
| `subprograms/` | Calls, parameters, overloads, operators and conventions |
| `syntax/` | Lexical/parser boundaries and declaration diagnostics |
| `tagged/` | Dispatch, class-wide types, interfaces and primitive operations |
| `tasks/` | Tasks, protected objects, masters, delays and shared scalars |
| `types/` | Scalar/record types, discriminants, derivation and attributes |
| `units/` | Packages, visibility, renaming, contexts and separate compilation |

`units/loading/`, `units/separate/` and `elaboration/pragmas/` retain isolated
multi-unit scenarios. Upstream ACATS sources remain in `../acats/`; benchmarks
and C/C++ runtime tests remain separate from these fixtures.

Register new tests explicitly in [../CMakeLists.txt](../CMakeLists.txt), using a
path such as `arrays/example.adb`. Mixed-feature CMake loops may resolve a
unique fixture basename through `ada_test_fixture`; ambiguous basenames require
an explicit path. Companion files use the existing test name, so CTest names,
filters and build work directories do not depend on the source folder.

Run all tests from the repository root:

```sh
ctest --test-dir build --output-on-failure
```

Select existing test names with `-R`, for example
`ctest --test-dir build -R 'array|controlled|elaboration' --output-on-failure`.
Moving a fixture does not change its test name or automatically register a test.
