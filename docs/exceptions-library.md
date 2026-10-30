# Ada.Exceptions

Exception names now contain their enclosing named scopes, beginning at the root
library unit, in uppercase. An exception renaming keeps the original identity
and name. Standard exceptions retain their defining names; I/O exceptions use
`ADA.IO_EXCEPTIONS.<NAME>`. Unhandled-exception diagnostics and
Exception_Information use these same names. Names in unnamed blocks follow the
compiler's enclosing-scope naming convention.

`Wide_Exception_Name` and `Wide_Wide_Exception_Name` provide both identity and
occurrence overloads, with lower bound 1. Null identities and occurrences raise
Constraint_Error, as for Exception_Name. Their results use Standard
Wide_Character/Wide_String (16-bit elements) and
Wide_Wide_Character/Wide_Wide_String (32-bit elements). Contextual literals,
array storage, comparisons and character position/value operations support the
name APIs. This prerequisite does not complete the later wide-library milestone:
Unicode identifier/source encoding rules, wide I/O and the full wide-character
facilities remain open. Wide character Image/Value/Width currently diagnose that
gap. Current exception identifiers use the existing ASCII spelling rules.

## Occurrence ownership and results

Exception_Occurrence remains limited private. Ordinary assignment, copy
initialization and simple returns of existing occurrences are rejected. A
function can construct an occurrence with an extended return and can forward
another occurrence-valued function or Input result:

```ada
return Result : Ada.Exceptions.Exception_Occurrence do
    Ada.Exceptions.Save_Occurrence (Result, Source);
end return;
```

Construction uses the normal limited-result ABI and caller-owned storage.
Occurrences also work as components and allocated results. Handler bindings
remain read-only and cannot escape by copying. Save_Occurrence's procedure still
preserves up to 200 message characters without allocating; its allocating
function preserves the full message in the returned allocation. Reraising copies
the message before the source's storage can be released.

Streamed occurrences own messages longer than 200 characters. Their inline
buffer handles shorter messages; replacement releases prior owned storage.
Compiler-generated finalization registrations release owned messages at scope
exit, on failed construction and during deallocation, including components and
function results. This is implementation ownership, not a user-visible
Controlled derivation. Handler snapshots continue to use their enclosing
runtime arena.

The private C/Ada occurrence layout now occupies 760 bytes, with the owned
message pointer at offset 752. Rebuild compiled Ada units when updating the
compiler/runtime together.

## Streams and partitions

The public Read_Exception_Occurrence and Write_Exception_Occurrence procedures
implement the type's Read/Write attributes. Default Input/Output use them.
The format consists of two String'Output values: expanded exception name, then
message. An empty pair represents Null_Occurrence. Message bounds normalize to
1 and embedded NUL characters are preserved. Reading completes the frame before
replacing the destination; invalid null-name/nonempty-message combinations raise
Data_Error. Premature input uses the stream layer's End_Error behavior.

No exception pointers, owned-storage pointers or traceback addresses enter the
stream. A reader resolves known names to its local declarations and standard
exceptions. Unknown names are interned as stable process-local exception
identities, allowing receipt and reraising of an occurrence from another
partition. The registry is synchronized and its imported identities live until
process shutdown. Streamed occurrences retain identity and the full message;
origin/trace snapshots are deliberately omitted from the external format.
The format follows AIDA's stream representation and is not a portable schema
for other Ada implementations.

## Evidence

* `exceptionlibrary`: expanded and aliased names, both wide APIs, null checks,
  wide storage/positions, extended/forwarded/allocated results, 5,000-character
  messages with NUL bytes, streaming, replacement and reraising after file close.
* `exceptionlibrarytasking`: four tasks repeatedly use nested memory streams,
  forwarded occurrence results and message cleanup.
* Both scenarios run whole-program, per-unit and determinism checks.
  `runtime.exception_partitions` uses distinct producer/consumer executables to
  check reconstruction with the consumer's local identity.
* `runtime.exception_library` checks registered/imported identities, malformed
  names, full/inline message replacement, release and four concurrent readers.
  Existing message, occurrence, handler, cleanup and readonly rejection tests
  remain enabled.
* Unchanged pinned ACATS CB41001, CB41003 and CB41004 execute as PASS.
  CB41002 was assessed but remains blocked by existing unsupported task-entry
  syntax; it is not counted as a pass.

Focused ASan/UBSan checks cover the compiler, C ownership/registry, generated
programs and separate partitions; the concurrent C registry test also passes
TSan. Generated Ada memory accesses remain
uninstrumented, and LeakSanitizer is unavailable on this host. The new C runtime
unit cross-compiles with Windows/MinGW; native Windows execution remains a
separate validation step.

The API and contracts follow [Ada 2005 RM 11.4.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-11-4-1.html).
