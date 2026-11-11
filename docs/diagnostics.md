# Compiler diagnostics

Both `ada` and `adac` accept:

- `--diagnostic-format=auto|plain|rich` (default `auto`). Rich diagnostics add
  numbered source lines, an arrow naming the source location and caret/tilde
  span markers. Auto selects rich output for a terminal, except `TERM=dumb`.
- `--color=auto|always|never` (default `auto`). Auto colors terminal output
  unless `NO_COLOR` is present or `TERM=dumb`. Explicit overrides take precedence.
  Color is independent of the source-snippet format; labels and markers also
  convey severity and location without color.

Redirected output defaults to the existing `file:line:column: severity: message`
format without escape sequences. The same header remains in rich output.
Related declarations and generic instantiations use separate `note` headers;
consumers must distinguish these from errors. Command-line and host-tool failures
can still use their own short messages without source snippets.

For example:

```text
types.adb:11:10: error: the assigned value has an incompatible type; expected 'Count', found 'Other'
   --> types.adb:11:10
   |
11 |     X := Y;
   |          ^
types.adb:4:5: note: assignment target is declared here
  --> types.adb:4:5
  |
4 |     X : Count := 1;
  |     ^
```

## Locations and source retention

Header columns are one-based Unicode scalar positions, as used by the lexer;
for compatibility a tab counts as one source column. Markers use display cells:
8-cell tab stops, combining marks of width zero, wide/fullwidth characters of
width two and ambiguous characters of width one. The diagnostic-only width table
uses Unicode 17.0.0; the language's Unicode 4.0 identifier rules are unchanged.
`tools/generate_diagnostic_width.py` records how to reproduce the table. Emoji
sequences and terminal-specific font shaping can differ from scalar widths.
Control bytes, invalid UTF-8 and format controls are escaped in source snippets.
CRLF, LF, BOMs and the lexer's Unicode line separators retain matching locations.

The lexer records exclusive token ends against an owned source snapshot. Literal
spans use original spelling, including quotes and doubled quotes, rather than
decoded values. Diagnostics with only a point underline one cell. An explicit
span can cover multiple lines; long spans show their first two lines and last
line. Long source lines are clipped around the location, with ellipses. Width is
read from `COLUMNS`, or the terminal when available, with a default of 100 and
bounds of 40–240 cells. Headers/messages remain unwrapped for log parsers.

Source text is never reread during reporting. Separately loaded source units
retain their own snapshots, locations and spans. Generic token replay uses the
original locations and emits nested instantiation notes, innermost first. A note
at the primary location is omitted. Source-free `.asi` imports preserve original
file/line/column headers but have no source snapshot, so they omit snippets;
missing files and invalid locations also fall back to headers. The interface
format and semantic identities are unchanged.

## Phase 10 audit and recovery

The rejection suite was reviewed with primary errors distinguished from related
notes. The focused improvements cover:

- Initializers, assignments, returns and parameter defaults report expected and
  actual types and point to the value. Notes identify the object subtype,
  assignment target, function or parameter declaration.
- Same-spelling types use package qualification when available; otherwise the
  message explains that their declarations differ. Anonymous and universal
  types have readable descriptions instead of internal generated names.
- Duplicate objects, numbers and exceptions point to the previous declaration.
  Ambiguous overloadable bare names show two possible declarations; a single
  unsuitable interpretation includes available expected/actual types.
- Instantiation context is scoped across generic validation and replay, including
  defaults diagnosed in a separately loaded generic declaration.
- Declaration recovery preserves a following declaration or enclosing delimiter.
  Statement recovery preserves `end`, `else`, handler and select delimiters.
  Malformed pragmas still synchronize through their semicolon. Existing progress,
  nesting and error-count limits remain in place; suppressed errors do not emit
  detached notes.

This is a diagnostics-quality milestone for the supported compiler surface, not
an Ada conformance claim or a promise of a dedicated explanation for every
possible overload failure. Existing unsupported-construct and legality checks
remain active.

## Validation

`diagnostics.contract` asserts exact messages, files, lines, columns and a rich
rendering snapshot, with legal counterparts and separate-unit/generic controls.
It covers plain/rich output, forced/automatic color, redirected output, pseudo
terminals on POSIX, tabs, wide/combining characters, CRLF/BOM, multiline spans,
clipping, unavailable source, snapshot ownership, nested contexts and the
100-error reporting limit. It also checks frontend/driver option forwarding.

The existing rejection snapshots preserve independent primary errors and their
source lines. Only two misleading parser cascades were removed (unterminated
string recovery and a missing labelled statement). ACATS policy B43002D accepts
the same type-error family with its new expected/actual detail. Related notes
cannot satisfy B-test markers; legal controls still require empty compiler
output. The selected grades remain 17 PASS and nine known FAIL; refreshed raw
B43002D/B64002A observations retain their ungraded rejection outcomes.

Local validation on Darwin arm64 (11 November 2026):

- `ctest --test-dir build -j 8 --output-on-failure`: **2,114/2,114 passed**,
  including the existing legality, ACATS, separate-unit, determinism, runtime and
  host integration controls.
- `ctest --test-dir build/validation/asan-ubsan -R
  '^diagnostics\.|^error\.|robustness.malformed_fuzz' -j 2 --output-on-failure`:
  **306/306 passed**, including all 304 existing rejection tests and the
  malformed-source campaign.
- The strict B-test grading check retains **17 PASS / nine known FAIL**, with no
  drift in its reviewed results. The raw baseline refresh covers all 26 B-test
  rejections and five existing C-test rejections; only two diagnostic snapshots
  change, with no outcome changes.
- The generated Unicode-width header reproduces byte for byte. `git diff --check`
  passes, and `qbe/` is unchanged.

Other native hosts retain the Phase 12 validation owner.
