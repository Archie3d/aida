# Wide text I/O (Phase 7)

Ada.Wide_Text_IO and Ada.Wide_Wide_Text_IO implement the Ada 2005
[A.11 interfaces](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-A-11.html):
file management, standard/current input/output/error, characters, strings,
bounded and function-valued Get_Line, Look_Ahead, both Get_Immediate forms,
Flush, positions and line/page control. Names and forms remain ordinary String;
text items use native Wide_String or Wide_Wide_String elements.

Integer_IO, Modular_IO, Float_IO, Fixed_IO, Decimal_IO and Enumeration_IO are
nested generics. Integer, Long_Integer, Float and Long_Float each have the
standard nongeneric instances in both widths, for example
Ada.Integer_Wide_Text_IO and Ada.Long_Float_Wide_Wide_Text_IO. The Ada bodies
use the same numeric algorithms and checked parsers as narrow Text_IO; wide
string parameters cross the C ABI with their actual cell widths. Enumeration
images and values use the wide scalar attributes. Lower_Case output uses
pinned Unicode 4.0 simple lowercase mappings and preserves quoted characters.

## External representation and positions

External wide text is strict UTF-8, independent of the host locale. A character
occupies one native element and advances Col once, including a supplementary
character in Wide_Wide_Text_IO. Output wraps and checks field widths in
characters. Input normalizes CRLF and CR to LF; LF and form-feed delimit lines
and pages. Get skips these terminators, while Get_Line returns preceding data.
A full bounded line buffer leaves the terminator for the next operation. Last
uses the actual target bounds; null buffers do not consume input. Function
results have lower bound one and preserve embedded NUL characters.

Malformed, truncated, overlong and surrogate UTF-8 sequences raise Data_Error.
A valid supplementary input character raises Data_Error in Wide_Text_IO,
whose element type cannot represent it. Output rejects surrogate values and
Wide_Wide_Character values above U+10FFFF with Data_Error before writing that
item. Other Unicode scalar values, including nongraphic values constructed
with Val, can be encoded. A BOM in an external file is an ordinary U+FEFF data
character. This choice does not change the separate UTF-8 source-file policy.

Get_Immediate returns control characters without CRLF normalization and does
not change logical positions. Its Available form polls for a complete character;
an incomplete UTF-8 sequence is retained in the file until another immediate or
ordinary read completes it. EOF in such a sequence is Data_Error. The existing
POSIX terminal and Windows console/pipe mechanisms supply external bytes;
terminal encoding configuration and native Windows terminal execution remain
part of the cross-host audit.

## File lifetime and synchronization

The wide families share the existing file pool, pinning, per-file locks,
reset/append behavior and owner-address finalization described in the
[narrow text-I/O contract](text-io.md). Their standard/current selections and
logical standard-file positions are separate from narrow Text_IO and from
each other. Closing or finalizing a selected file restores that family's
standard selection. Standard/current results are borrowed snapshots, including
forwarding through Ada functions and use as temporary actual parameters; they
never close their owner's file. Result storage is registered when the caller
has not supplied a registration, so a borrowed variable subsequently closed
and reopened as an owner still gets scope cleanup. Owning records, arrays and heap objects use
the ordinary compiler cleanup path.

Files use the same LF/form-feed final-page termination as narrow text. The
shared pool permits 32 simultaneous nonstandard files across all I/O families.
Empty Form is supported; other forms raise Use_Error. Names are host path bytes
with the existing 1,023-byte limit. Public counts/string lengths remain signed
32-bit values. Unicode does not change the ownership or encoding of binary
Sequential_IO, Direct_IO or Stream_IO files.

## Validation and remaining work

`wideiolibrary` and `widewideiolibrary` execute whole-program and per-unit modes
and compare deterministic IR. They cover native-width and supplementary text,
all nested generics and standard instances, Unicode enumeration casing, actual
string bounds, null/full line buffers, NUL-containing owned line results,
layout, selected/borrowed files, borrowed-to-owning close/reopen transitions
and repeated scope finalization. Fixed-width numeric input rejects non-ASCII
characters even when their low byte spells a digit. The unchanged
ACATS CXAB001 executes wide character/string file operations, reset, append,
open/close and deletion; provenance is in tests/acats/README.md.

`runtime.wide_text_io` checks exact UTF-8 file bytes, invalid encodings and
unrepresentable values, partial nonblocking pipe reads, ordinary-read recovery,
character positions, field failure atomicity, selection cleanup and four-thread
file ownership. Existing narrow I/O and borrowed-result regressions remain in
the full suite. Sanitizer checks cover the compiler and C runtime; generated
QBE memory accesses are uninstrumented.

Wide string/map libraries, their bounded/unbounded text-I/O children and the
remaining wide attribute-subprogram/legality audit retain their next roadmap
steps. No claim of full predefined-library conformance is made here.
