# Calendar and host services

Ada 2005 references: [9.6 Calendar](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-6.html),
[A.16 Directories](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-A-16.html),
[A.17 Environment_Variables](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-A-17.html).
This increment implements the base Calendar prerequisite and the Ada 2005
Directories and Environment_Variables APIs. Calendar's arithmetic, formatting
and time-zone child packages remain open in Phase 7. The later two-argument
Environment_Variables.Value overload is outside the Ada 2005 target.

## Calendar

Time is a private signed 64-bit count of microseconds since the Unix epoch.
The supported range is 1901-01-01 through 2399-12-31, inclusive. Clock uses the
host realtime clock. Split, Year, Month, Day, Seconds and Time_Of use UTC and
the Gregorian calendar, independent of TZ, locale and daylight-saving changes.
Leap seconds are not represented. Duration conversions round to the nearest
microsecond, with ties away from zero; the existing Duration scale is unchanged.
Invalid civil dates and unrepresentable Time/Duration arithmetic raise
Time_Error; public subtype violations raise Constraint_Error. Seconds = 86400
normalizes to the next day when that day is representable. Clock failure raises
Program_Error. The compatibility package Calendar renames Ada.Calendar.

Calendar absolute delays recheck the realtime clock after monotonic sleeps of
at most 100 ms, observing clock changes without promising hard realtime
scheduling. Expired delays return immediately. As with other delays, a delay
inside a protected action raises Program_Error even if already expired.

## Environment variables

On POSIX hosts the package reads and changes the actual process environment;
there is no persistent cache. External changes made before a call are visible.
Names and values are host bytes, case-sensitive; names must be nonempty and
contain neither NUL nor '='. Values may be empty but may not contain NUL.
Invalid Set/Value/Clear arguments and a missing Value raise Constraint_Error;
Exists returns False for invalid or missing names. An empty value is distinct
from an absent variable. Clear(Name) on an absent valid name is a no-op.
Clear without arguments clears the process environment, including inherited
variables. Allocation failure raises Storage_Error. Malformed inherited entries
without a usable name prevent Clear-all with Program_Error.

Iterate visits the host environment in its current order, including duplicate
names if inherited from outside the package. Value chooses the host getenv
value for duplicate names. Callback exceptions propagate. No runtime lock is
held during callbacks. As required by A.17, clients must serialize mutations
against all other environment operations, including foreign getenv/setenv/
putenv calls; mutation from an Iterate callback is erroneous. Concurrent readers
are supported when nobody changes the environment. Subprocesses inherit changes
through the normal host mechanisms.

## Directories

Host adapters cover POSIX file systems (macOS and Linux) and native Windows
under MSYS2/MinGW. POSIX names are host bytes with '/' separators; empty names
and embedded NUL raise Name_Error. The Windows path and encoding rules are
below. There is no fixed path-size buffer. All nonempty Form strings raise
Use_Error; no options are silently discarded. Native MSYS2 execution validation
remains open in Phase 8.

On POSIX, Full_Name returns an absolute name, resolves existing symlink prefixes before
normalizing '..', and permits a missing suffix. The other name operations are
lexical and do not require files to exist. Trailing separators are removed
except for '/'; Simple_Name('/') and Base_Name('/') are '/'.
Containing_Directory of a simple name is '.', while '/' has no containing
directory and raises Use_Error. Extension uses the final dot, excluding an
initial dot and the special names '.' and '..'; an absent extension returns an
empty string. Compose accepts a simple name and an optional extension without
'/' or '.', inserting separators as needed. All returned strings start at 1.

Create_Path creates missing parent directories, accepting existing directories.
Creation uses host permissions filtered by umask. Delete_Directory requires an
empty directory. Delete_File accepts ordinary/special files, not directories.
Queries follow symlinks; dangling symlinks do not Exist. Size is the host byte
size (0 .. 2**63-1); for directories/special files it is the host stat value.
Modification_Time uses the host timestamp rounded down to microseconds and
raises Use_Error if it cannot be represented in Calendar's range. Such an
entry can still be enumerated and queried for its name, kind and size.

Copy_File copies ordinary-file contents, overwriting an existing ordinary
file; it rejects directories with Name_Error and unsupported special files
with Use_Error. It rejects the same inode (including hard links) before
truncation. A failed copy may leave a partially written target. Rename never
overwrites an existing target: macOS uses renamex_np(RENAME_EXCL), Linux uses
renameat2(RENAME_NOREPLACE), and other hosts without an atomic no-replace
operation raise Use_Error. Windows uses MoveFileExW without replacement or
cross-volume-copy flags. Recursive deletion uses descriptor-relative,
no-follow traversal on POSIX: symlinks within the tree are unlinked rather
than traversed. A symlink as the root is refused. A failed deletion may leave
part of the tree deleted. File operations are not transactions against external
file-system changes.

Missing/non-directory path components, malformed names, excessive names and
symlink loops normally raise Name_Error. Host permission, nonempty-directory,
unsupported-operation and other environmental refusals raise Use_Error; I/O
hardware errors raise Device_Error. Query size overflow raises Constraint_Error.
Current directory is process-wide: callers must serialize Set_Directory against
operations using relative names, including other Ada I/O and foreign code.
Independent operations on absolute paths can run from different tasks.

## Windows (MSYS2/MinGW)

The Windows adapter uses wide Win32 APIs internally. Public String values are
encoded using the process ANSI code page (CP_ACP), including UTF-8 when Windows
has configured that as the process code page. Invalid input encodings raise
Name_Error for paths or Constraint_Error for environment strings. A name or
value that cannot be returned without best-fit/replacement characters raises
Constraint_Error. This keeps byte-string file names consistent with the CRT
file operations used by Ada.Text_IO; it does not introduce wide Ada types.

Both '/' and '\' are accepted as input separators. Returned paths use '/'.
Drive-absolute, drive-relative and UNC server/share names are supported.
Full_Name uses GetFullPathNameW and can name files that do not exist; Windows
performs lexical dot/dot-dot normalization before resolving filesystem links.
Roots retain their drive or UNC share; Containing_Directory of a root raises
Use_Error. Compose preserves the distinction between C:file and C:/file.
Explicit Win32 device namespaces, alternate data streams, reserved DOS device
components and components ending in a dot or space are rejected with Name_Error.
Internally generated extended-length paths are used for filesystem calls;
Windows path-length, permissions and filesystem restrictions still apply.

Queries open files with sharing for reads, writes and deletion, follow reparse
points, and cache handle metadata. Modification times convert FILETIME's
100-nanosecond intervals since 1601 to UTC microseconds since 1970. Copy uses
read/write handles, compares volume/file identities before truncation (including
hard links), and reports partial read/write/close failures. Rename refuses an
existing target. Delete_Tree opens directory reparse points without following
them, obtains canonical ancestor paths from the opened handles, denies directory
renaming during traversal, and deletes directories through those same handles.
A root junction/symlink is refused; junctions/symlinks within the tree are removed
without visiting their targets. Read-only/locked files or unsupported reparse
operations can raise Use_Error after partial deletion, as on POSIX.

Environment names are case-insensitive. The native process environment is the
source of truth; changes made by foreign Win32 setters or CRT _putenv calls are
visible. Set/Clear synchronize the MinGW CRT environment too. Native empty
values are retained and differ from absent values; because the CRT represents
empty values as absence, foreign getenv/_spawn may see an empty Ada-set value
as absent. Foreign Win32 changes may leave the CRT's own cached getenv view
stale, but Ada operations read the current native block. CreateProcess with an
inherited native environment sees the actual native empty values. Drive-private
entries such as =C: are excluded from Iterate/Count/Clear-all, preserving
Windows' per-drive directory state. The same caller-synchronization rules as
POSIX apply to mutations. Callbacks run after all native snapshot buffers used
for their arguments have been copied into owned Ada results.

Search snapshots use FindFirstFileW/FindNextFileW for enumeration, then the
adapter's own '*'/'?' matcher. Windows matching uses ordinal case-insensitive
comparison on UTF-16 code units; brackets are literal. Filtered metadata follows
links, and dangling links/disappearing entries are omitted. No find/file handle
is retained by a successful snapshot or across a callback.

## Search ownership

Start_Search first ends any previous search, then snapshots the matching entry
names and stat metadata. Pattern is a byte glob: '*' matches any sequence,
'?' matches one byte, and other bytes (including brackets) are literal. A null
pattern matches everything. '/' and NUL are invalid in a pattern. Filters select
the three file kinds. '.' and '..' are omitted. Dangling symlinks and entries
that disappear during collection are omitted; other collection failures raise.
Order is host enumeration order, with no sorting guarantee. Once collected,
metadata and iteration order are unaffected by later directory changes.

Default/ended searches have no entries; Get_Next_Entry then raises Status_Error.
An invalid Directory_Entry_Type raises Status_Error on all queries. An entry
owns its name and cached metadata independently of the search, and remains
usable after End_Search. Search and entry types are limited; controlled
components release snapshots and names on normal exit and exception unwinding.
Search callbacks can nest another search, end a separately held search, or raise
without leaking handles. No callback executes under a runtime lock. A single
search/entry object requires caller synchronization if shared by tasks.

C entry points use concrete scalar types. Host string results use the existing
owned array-result descriptor; generated code checks exceptions before adopting
the buffer into its expression arena. No temporary string uses a global or
per-task reusable buffer. Search snapshot memory belongs to its limited
controlled guard and can be finalized by a different task after synchronization.

## Validation

`calendarlibrary`, `environmentvariables`, `directorieslibrary` and
`hostlibrarytasking` run as whole programs, separate units and determinism
checks. `hostlibraryerrors` pins private/limited legality diagnostics.
`runtime.host_library` exercises calendar boundaries, concurrent readers and
searches, owned results, hard-link copy refusal, symlink-aware names and
recursive deletion. Focused compiler/C-runtime ASan/UBSan and C-runtime TSan
cover the same increment; generated Ada accesses remain uninstrumented.
The host-library Ada cases and CXAH001/CXAH002/CXAG001 are enabled for MinGW as
well as POSIX. `runtime.windows_host_library` adds native environment/CRT
interop, drive/UNC names, hard-link copy protection and junction deletion checks;
these Windows execution checks are pending a native run (the dedicated C
check requires a filesystem supporting hard links and junctions, normally NTFS).
The Windows adapter and native test cross-compile/link against LLVM-MinGW; that is not a Windows
execution result. Unchanged ACATS CXAH001/CXAH002 exercise environment reads,
writes and callbacks;
C96006A/C96007A exercise Calendar comparisons, subtype checks and invalid dates.
Unchanged CXAG001 also passes, covering directory/file operations together with
Text_IO string Get and the local REPORT.Legal_File_Name adapter.
These selections do not establish conformance.
