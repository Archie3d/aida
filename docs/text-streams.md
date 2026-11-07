# Text-file streams (Ada 2005 A.12.2–A.12.4)

Ada.Text_IO.Text_Streams, Ada.Wide_Text_IO.Text_Streams and
Ada.Wide_Wide_Text_IO.Text_Streams supply the required distinct Stream_Access
types and `Stream (File : File_Type) return Stream_Access` functions.
The [RM contract](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-A-12-2.html)
permits mixing text and binary operations; stream operations leave column,
line and page counts unchanged. The child bodies reuse the ordinary locked
Stream_IO adapter, including its library-level tag and Read/Write primitives.
Obtaining a view neither allocates a second file nor transfers ownership.

## Bytes, text and lookahead

Stream elements are external bytes for all three families. Raw transfers do
not normalize CR/LF or transcode UTF-8. Stream attributes retain their
[type-aware binary format](streaming.md); Wide_Character'Write, for example,
is a binary value transfer rather than Wide_Text_IO.Put's UTF-8 encoding.

Text lookahead retains both decoded symbols and the exact bytes that produced
them. A raw read first consumes this saved input, then any partial nonblocking
UTF-8 prefix, byte lookahead/replay and the OS stream. No seek is required, so
pipes work too. CR/LF remains two bytes when returned through a stream even
though ordinary text input sees one newline. Form feeds and multi-byte symbols
are likewise preserved. An empty raw read leaves all lookahead intact.

Switching back to text resumes decoding the remaining bytes. Consuming only
part of a UTF-8 character through a stream can leave an invalid continuation
for the text decoder, which raises Data_Error. Bytes retained by failed text
lookahead can be recovered through a raw read. Reset discards all lookahead,
replay and incomplete decoder state. These rules apply to narrow byte text and
to strict UTF-8 wide/wide-wide text; Wide_Text_IO still rejects non-BMP symbols.

Raw reads and writes do not update text counts. The next text operation updates
counts only for the symbols it consumes or emits. Raw output marks the file as
not page-terminated; normal Text_IO reset/close termination rules still apply.
Mode checks precede any transfer: wrong-direction operations raise Mode_Error,
and Stream on an unopened/closed file raises Status_Error. Element-array reads
return a short Last at EOF; incomplete scalar stream reads raise End_Error.

## Identity, lifetime and concurrency

Repeated Stream calls for one file return the same adapter, including through
borrowed Current_Input/Output/Error results. Explicit files, standard files and
selected current files use the same runtime file pin and mutex protocol.
Reset/mode changes preserve adapter identity while retaining the parent text
package's existing reset and selected-file rules. Finalizing a borrowed handle
cannot close its owner. Closing/finalizing the actual file invalidates every
stream alias; later use, including after a pool slot is reused, is erroneous
under the inherited A.12.1 lifetime rule. No generation-safety guarantee is made.

Each raw transfer and text operation holds the file lock while updating input
state or transferring output. Independent files and concurrent calls on a
shared file are covered. Standard input/output/error wrappers remain distinct
between text families; interleaving different families or foreign C I/O on the
same external standard handle does not coordinate their separate lookahead.
Text codec, file-pool, Form, terminal and native Windows/MSYS2 execution limits
remain those of the [narrow](text-io.md) and [wide](wide-text-io.md) contracts.

## Evidence and a separate equality defect

`textstreams`, `widetextstreams` and `widewidetextstreams` cover exact replay,
CR/LF/form-feed, UTF-8 splits and recovery, null/shifted buffers, unchanged text
counts, stream identity, reset/mode errors, borrowed current handles and four
Ada workers. `textstreamsvalues` round-trips scalar/array unbounded-string
contents for all three widths, checking logical contents per component.
All four programs have whole/separate/deterministic controls. Standard-stream
checks run with both file-backed and piped stdin, exercise stdout/stderr, and
check family-specific argument/access-type diagnostics.

`runtime.text_streams` adds four native workers, shared-file raw/text writers,
partial nonblocking UTF-8 prefixes, non-BMP rejection and byte recovery,
termination, borrowed ownership and closed-file checks. Existing text, wide,
terminal, pipe and stream regressions remain active. All 122 inventoried units
now resolve, with no missing-unit or original API-gap guards remaining.

Unchanged ACATS CXAC004 compiles and reaches its complete scenario but remains
RUN_FAILED: predefined equality of arrays of Unbounded_String compares private
storage pointers even though streamed string contents round-trip correctly.
This existing composite-equality defect is tracked separately in TODO.md.
The [manifest](../tests/acats/text_streams/manifest.json) pins original bytes,
revision, support and exact observed output. `baseline.text_streams_acats`
checks that observation; matching the failure is not a conformance pass.

Validation on Darwin arm64: Release 1,894/1,894 tests, 20 focused ASan/UBSan
checks and one native TSan check pass. All 145 original backfill observations
and the compiler fingerprint remain unchanged. ASan/UBSan instrument compiler
and runtime code; generated QBE accesses are not ASan-instrumented.
