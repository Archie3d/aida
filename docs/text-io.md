# Text I/O and file lifetime

The narrow-character `Ada.Text_IO` API follows Ada 2005
[A.10.1–A.10.12](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-A-10-1.html).
This is the supported byte-string library, not a claim of complete predefined
library conformance. [Wide I/O](wide-text-io.md) now uses the same file layer;
wide string adapters and the broader environment audit retain their roadmap owners.

## API and text representation

The package includes current input/output/error selection, Flush, line/page
lengths and positions, line/page control, Look_Ahead, both Get_Immediate forms,
and bounded-buffer and String-valued Get_Line. Numeric and enumeration I/O are
nested generics, as required by A.10.1; existing AIDA child-style with clauses
remain accepted as compatibility aliases of `with Ada.Text_IO`.
`Ada.Long_Integer_Text_IO` and `Ada.Long_Float_Text_IO` supplement the Integer
and Float instances. `Ada.Text_IO.Bounded_IO` and `Unbounded_IO` supply the
Ada 2005 string adapters, including whole-line consumption before bounded
conversion (which can raise Length_Error).

The host representation uses bytes, LF line terminators and form-feed page
terminators. CRLF and standalone CR normalize to LF on ordinary text input;
immediate input returns control bytes directly. Closing or resetting an output
text file terminates its final page. Binary Sequential_IO, Direct_IO and
Stream_IO files use separate constructors and never gain text terminators.
Append does not insert a separator before the newly appended text.

A bounded-buffer Get_Line stops immediately when its buffer fills, leaving the
terminator for the next operation. Last uses the caller's actual bounds,
including null buffers and slices. String-valued Get_Line returns a lower bound
of one and retains embedded NUL bytes. Logical lookahead uses a queue rather
than relying on multiple C ungetc calls. Col/Line/Page use wider internal
counters and raise Layout_Error if the result exceeds Count'Last. Scalar fields
move to a fresh line when needed; fields exceeding a bounded line length raise
Layout_Error. String Put wraps by character.

On POSIX terminals, immediate input temporarily disables canonical input and
echo, restoring the settings on every exit. Its Available form polls without
waiting. Windows console input uses the console byte APIs; pipe availability
uses PeekNamedPipe. The runtime operates on bytes, with no Unicode decoding or
terminal-key translation. Terminal behavior and native Windows execution remain
part of the cross-host audit.

## Numeric and enumeration input

Integer, modular, floating, ordinary fixed and decimal input share a lexical
candidate scanner. Width zero skips leading text blanks/terminators and leaves
the following delimiter or adjacent signed item unread. Nonzero Width includes
leading blanks, stops at the requested width or line boundary, and validates
the whole field. Buffers grow with the input rather than truncating at 64 bytes.
String Get overloads return the actual last-consumed index; invalid syntax,
range failures and incomplete exponents produce Data_Error. File exhaustion and
host read failures remain End_Error and Device_Error respectively.

Signed integer and floating input reuse the existing exact value parsers;
floating values round directly to the instantiated binary format. Modular
input retains the base range rule, and fixed/decimal input uses the existing
exact scaled parser. Enumeration input recognizes identifiers or quoted
graphic character literals without consuming following punctuation. Control
characters use names such as HT; quoted controls raise Data_Error. Enumeration output
preserves the case of quoted characters. Floating output supports large Aft
fields and signed zero; its host printf rounding/locale behavior retains the
Phase 8 floating-format audit (in particular decimal halfway rounding).

## Ownership and finalization

All four File_Type families remain limited private records containing an
opaque eight-byte handle. The implementation-only `Ada.IO_Handles.File_Handle`
component participates in the ordinary compiler finalization chain, including
records, arrays, allocators, extended returns, exceptional exits, collections
and library finalization. A successful Open/Create records the address of its
owning object. Finalization closes only a still-open file owned by that exact
address; unopened, failed-open, closed and borrowed handles are harmless.

Standard/current file results are borrowed snapshots. Selection changes do not
change a saved snapshot, and disposing of a snapshot does not close its owner.
The compiler adapts these imported borrowed results to Ada limited-result
storage, including forwarding through an Ada function. This exception does not
add a general ownership ABI for imported controlled results. Stream access
values remain borrowed adapters and become invalid when their file closes.
Explicit Close/Delete detach the owner's handle even when called through a
borrowed view. Reusing a file requires the normal synchronization and lifetime
rules; borrowed handles do not extend an owner's lifetime.

Each runtime operation pins and locks its file. Selection and pool changes use
the shared table lock; blocking reads/writes, close/flush and text termination
do not retain that table lock. The current error selection belongs to the
environment context. Finalizers use the same synchronization and abort-deferral
machinery as explicit operations. Concurrent operations on independent files,
including cleanup in different tasks, share the existing access collections.

## Host limits and evidence

There are 32 simultaneous nonstandard open files. Names use host path bytes,
are limited to 1,023 bytes and reject embedded NUL. Empty names use anonymous
scratch files. Empty Form is the only supported form; other forms raise
Use_Error. Standard files cannot be closed/deleted through these operations.
String lengths and public text counts retain the signed 32-bit limit.
Sequential_IO and Direct_IO retain their documented element representation
subset; Stream_IO's remaining positioned overloads, Set_Mode and Flush belong
to the Phase 8 environment inventory.

`textinputgrammar`, `textiolibrary`, `textstringio`, `iofilelifetime` and
`iolibrarytasking` run whole/per-unit and determinism checks. They cover long
and based tokens, width, range/errors, near-Integer'Last bounds, all selection
forms, full/null/sliced line buffers, 5,000-byte/NUL lines, layout, four workers,
borrowed results, heap/array/record cleanup and limited returns. Existing
`textfileedges` and host-directory size fixtures now reflect final page
termination. `runtime.text_io_library` checks pipe availability, control bytes,
unterminated foreign lines, count overflow and owner/borrow cleanup.

Unchanged ACATS CE3402A, CE3704A, CE3804A and CE3904A execute as PASS. Focused
compiler and C-runtime ASan/UBSan checks and C-runtime TSan checks pass;
QBE-generated Ada accesses remain uninstrumented and host LeakSanitizer is
unavailable. Native Windows/terminal validation remains separate.
