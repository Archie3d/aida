# Predefined environment audit (Ada 2005)

This Phase 8 increment audits the current environment and repairs defects in
supported APIs, as agreed for its scope. It does not implement the wholly
missing library families. Their existing gaps stay explicit in the coverage
matrix and the checked [inventory](../tests/predefined/inventory.json).

## Inventory and boundaries

The inventory names all 109 source specifications and the two compiler builtins
Standard/System. Its baseline check compiles an independent import probe for
each of the 111 resolving units, checks that every runtime spec is accounted
for, and verifies exact normal missing-unit diagnostics for ten absent units.
Resolving a unit is not a claim that its complete API is implemented. Core and
compatibility units, selected annex subsets and implementation extensions are
identified separately.

| Missing unit/family | Ada 2005 reference | Existing gap |
| --- | --- | --- |
| Numerics.Float_Random; Discrete_Random | A.5.2 | Generator ownership/state, discrete profiles, resets and state images |
| Storage_IO | A.9 | Typed representation buffers and Read/Write |
| Tags.Generic_Dispatching_Constructor | 3.9 | Generic profiles and construction by tag |
| Text_IO.Text_Streams and both wide families | A.12.2–A.12.4 | Mixed byte/text operations, lookahead and UTF-8 state |
| System.Storage_Elements; Address_To_Access_Conversions | 13.7.1–13.7.2 | Storage types, address arithmetic and conversions |
| System.Storage_Pools | 13.11 | User-defined pools and allocator/deallocator integration |

The same baseline check pins 22 partial-API observations: missing System names
and callable address-ordering profiles, Stream_IO's positioned overloads,
Set_Mode/Flush, and Standard.ASCII. These rejection observations are gap guards,
not legality or conformance passes. A new implementation or changed diagnostic
requires inventory review rather than silently retaining an obsolete claim.
The System list includes missing numeric limits as well as configuration,
address, bit-order and priority declarations. Current numeric support is
Storage_Unit, the two maximum moduli, Fine_Delta and Tick. Ordinary address
comparison expressions and a callable profile/renaming are distinct audit
surfaces; the latter remains recorded as a gap.

Ada 2012 additions are outside this language target. Specialized-needs annex
facilities retain their separate matrix scope, including full tasking/priority
services in Phase 9. The missing families and partial APIs now have explicit
[Phase 8 implementation items](../TODO.md#predefined-library-implementation),
added after the audit; those implementation items remain open.

## Supported API repairs

[RM 13.13.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-13-1.html)
defines Stream_Element as modular. It is now `mod 256`, with eight-bit storage;
modular arithmetic and formal modular-type matching work while the byte-stream
ABI remains unchanged.

Direct_IO and Stream_IO now declare distinct Count types, with Positive_Count
as a subtype of the corresponding Count. Direct_IO's internal imported helpers
use that type too; their C representation remains signed 32-bit. Different
Direct_IO instances have distinct Count identities. Clients needing operators
outside the defining scope must make them visible, just as for other
library-defined types. The existing streamrawedges fixture now has the required
`use type Ada.Streams.Stream_IO.Count`.

System.Tick is a universal-real named number, as required by
[RM 13.7(20)](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-7.html),
with the documented value one microsecond. Each receiving floating/fixed
context performs its own conversion; Duration rounds it to 1074 stored ticks.
Relative delay now adapts universal numeric expressions to Duration before
lowering, including named numbers such as System.Tick and half-tick expressions.
Without this adaptation, an accepted universal-real name could reach the
integer-duration runtime ABI as a floating bit pattern.

The [J.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-J-1.html)
root generic package renamings Direct_IO and Sequential_IO now expose the
already supported Ada children, alongside the existing Calendar, Text_IO,
IO_Exceptions and unchecked-operation compatibility views.

## Host and concurrent-call observations

The existing file/runtime contracts remain in force: byte text, LF/form-feed
termination, CR/LF handling, strict UTF-8 for wide text, signed 32-bit public
counts, 32 nonstandard open-file slots, opaque owned/borrowed handles and locked
per-file operations. Scratch files allow independent workers to avoid naming
collisions. Calendar is UTC with microsecond resolution and the previously
documented Gregorian range; leap seconds are unsupported.

Floating output uses host C formatting. The audit tests the C numeric locale
and FE_TONEAREST, including halfway rounding, negative zero and exponent widths,
from four independent threads with separate task contexts and buffers. On the
recorded host, halfway values use ties-to-even. Foreign code changing the C
numeric locale or floating rounding environment remains a host-formatting
boundary; this audit does not promise normalization of foreign state or Annex G
accuracy. Existing formatting tests retain their large-field/truncation checks.

The POSIX terminal test creates an actual pseudo-terminal. It checks empty
nonblocking input, observes canonical/echo suppression during a blocking
immediate read, delivers one byte without a line terminator, and verifies
persistent terminal settings and logical text positions afterwards. Configured
input translation and signal handling remain terminal choices. The kernel's
transient PENDIN state is excluded from persistent-flag comparison. Existing
pipe/control-byte and wide UTF-8 tests remain part of the focused evidence.
Native Windows console/MSYS2 execution and other targets remain in the existing
host-validation item; a POSIX pseudo-terminal pass is not evidence for them.

## Validation

`predefinedprofiles` exercises all corrected profiles, generic matching,
renamings, floating/fixed Tick contexts and delay, modular byte operations,
streams and both root generic I/O views. The main task and four workers repeat
independent scratch-file scenarios. Whole/separate/deterministic compilation
passes. `predefinedprofileerrors` rejects implicit Integer/Count/Stream_Element
mixing and cross-instance Count mixing. The standalone C test checks concurrent
formatting and POSIX terminal behavior. The inventory baseline covers unit
resolution, missing units and the 22 documented API gaps.

Unchanged CDD1001/CDD2B01 and C96008A/B retain executable PASS. The complete
145-case backfill refresh has no outcome drift: 107 PASS, 11 COMPILE_REJECTED,
one HARNESS_BLOCKED and 26 ungraded B rejections; only the compiler hash changes.

The final complete Release suite passes 1,797/1,797 checks. Fourteen distinct
focused checks pass under ASan/UBSan, including the final expanded inventory and
corrected stream fixture controls. The inventory confirms 111 resolving units,
ten missing-unit diagnostics and all 22 recorded API-gap diagnostics. The
145-case source hashes validate and every observation matches the preceding
snapshot; only its compiler fingerprint changes. Generated Ada memory accesses
remain uninstrumented and LeakSanitizer is disabled on this host.
