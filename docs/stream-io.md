# Stream files (Ada 2005 A.12.1)

Ada.Streams.Stream_IO supplies the complete
[A.12.1 profiles](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-A-12-1.html),
including positioned Read/Write, Set_Mode and Flush. Count is a distinct signed
32-bit Ada type with values 0 .. 2,147,483,647; Positive_Count starts at one.
Stream elements occupy one byte. The [type-aware streaming contract](streaming.md)
describes stream attributes and their native value format.

## Positions and modes

Each positioned operation holds the file mutex across validation, seeking and
transfer. Operations on different files can proceed independently. Mode_Error
is checked before positioning: Read requires In_File; Write and Flush require
Out_File or Append_File. Unopened/closed files raise Status_Error.

Create replaces the external file with an empty file. Open preserves existing
contents, including in Out_File. Open/Create start at index one, or at Size + 1
for Append_File. Reset retains contents and selects index one, or Size + 1 for
Append_File. Reset without Mode retains the current mode. Set_Mode preserves
the current index unless selecting Append_File, which selects Size + 1 even
when the previous mode was also Append_File. Neither Reset nor Set_Mode reopens
the file. A later Set_Index or positioned Write can overwrite existing bytes
in Append_File: append mode does not force every write to the end.

Read/Write advance the current index by the number of bytes actually transferred.
For positioned operations the new index is From/To plus that number; empty
positioned transfers still select the supplied position. Read supports shifted,
negative and sliced buffer bounds. Last is the last filled buffer index, or
Item'First - 1 when no bytes are read, including EOF and null buffers. A result
outside Stream_Element_Offset raises Constraint_Error. Element-array Read does
not raise End_Error at EOF; a stream attribute reading an incomplete value does.
EOF lookahead, Size and Index preserve the next logical byte.

Flush checks output mode, flushes host buffers without moving the index, and
reports Device_Error on failure. Mode changes and resets synchronize host
input/output state and clear EOF after successful positioning. Seek failures
raise Use_Error; transfer/flush failures raise Device_Error. Mode changes commit
the Ada mode only after host operations succeed.

## Ownership and host boundaries

File objects retain scoped/collection cleanup. Set_Mode and Reset preserve the
same runtime file, OS descriptor, owner and tagged stream adapter. Borrowed file
handles do not acquire ownership, and finalizing one cannot close the owner's
file. Stream(File) has stable identity across modes and resets. As specified by
RM A.12.1, closing/finalizing a file invalidates its stream aliases; using an old
alias after reopening is erroneous and has no generation-safety guarantee.

Named regular files prefer physical read/write access, with a one-way fallback
when permissions require it. A mode requiring unavailable physical access raises
Use_Error; changing permissions later does not upgrade an already open descriptor.
Pipes/devices use the requested direction. Sequential transfers and output Flush
work on nonpositionable streams; Index, Set_Index and positioned transfers raise
Use_Error. Size and Reset on nonpositionable files also raise Use_Error on this
implementation. Nonpositionable output can select Append_File without seeking.

The file pool retains its 32 nonstandard open-file limit. Nonempty Form is
unsupported (Use_Error). Public counts remain 32-bit although host seeks/tells
use 64-bit offsets. Writes extending a positionable stream beyond Count'Last
are rejected with Constraint_Error; Size rejects an external file beyond that
limit. An automatic next index of Count'Last + 1 cannot be returned by Index
and raises Constraint_Error. Sparse boundary tests do not allocate a 2 GiB
memory buffer. Native Windows/MSYS2 execution remains a separate host-validation
item; POSIX pipe, descriptor-failure and open-file rename evidence is host-specific.

## Evidence

`streamoperations` covers all added profiles, nontruncating open/reset, append
positioning, EOF/Last, null/sliced buffers, mode/range errors and stream identity.
`streamoperationstasking` checks positioned transfers through one file with four
Ada writers/readers. Both have whole/separate/deterministic compilation controls.
`runtime.stream_operations` adds four native workers, interference from mode/reset
calls, borrowed/owned cleanup, 32-bit sparse boundaries, nonpositionable streams,
flush failure and descriptor identity across a POSIX rename. Existing stream and
file-locking regressions remain active. The four API-gap guards are replaced by
executable controls in the predefined inventory.

Unchanged CXAC002, CXAC005, CXAC006, CXAC008 and CXAA019 pass complete REPORT
scenarios. Their [manifest](../tests/acats/stream_io/manifest.json) pins original
bytes, revision, support and CXAA019's exact expected informational output.
The harness still requires successful REPORT start/result markers and rejects
an expected-output file that omits them. These selected cases are evidence,
not a conformity assessment.

Validation on Darwin arm64: Release 1,879/1,879 tests, 18 focused ASan/UBSan
checks and one native TSan check pass. All 145 original backfill outcomes and
the compiler fingerprint are unchanged. ASan/UBSan instrument the compiler and
C runtime; generated QBE memory accesses are not ASan-instrumented.
