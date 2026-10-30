# Calendar children

Ada 2005 [9.6](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-6.html)
and [9.6.1](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-9-6-1.html)
define the target behavior. `Ada.Calendar.Arithmetic`, `Formatting` and
`Time_Zones` build on the existing base Calendar representation; they do not
change Time's ABI or the Duration scale. This is the documented calendar subset,
not a claim of complete predefined-library conformance.

## Time and offsets

Time remains a private signed 64-bit count of microseconds since the Unix epoch,
covering 1901-01-01 through 2399-12-31 inclusive. The Gregorian rules apply,
including the 2100/2200/2300 century exclusions and the 2000 leap day. Base
Calendar uses UTC independently of the host TZ, locale and daylight-saving
configuration. The compatibility package Calendar renames Ada.Calendar.

`Time_Zones.UTC_Time_Offset` returns zero because the implementation-defined
Calendar zone is UTC; the zone is always known. The default Date still evaluates
Clock normally. Formatting's explicit Time_Zone is a signed minute offset in
-1680 .. 1680: splitting adds the offset, composition subtracts it. Offset
arithmetic handles midnight, month and year boundaries before checking the
result's range. A split whose local year is outside Year_Number raises
Time_Error even when its UTC Time is representable. Calendar construction with
Seconds = 86400 or Sub_Second = 1 normalizes before the final range check.

Leap seconds remain unsupported on this POSIX-style clock representation.
Arithmetic.Difference returns zero Leap_Seconds, the Formatting.Split overloads
return False for Leap_Second, and both Formatting.Time_Of overloads raise
Time_Error when Leap_Second is True. These follow the RM's advice for a target
without leap-second support. UTC offsets are fixed offsets and do not consult
host timezone databases or DST rules.

## Arithmetic and decomposition

Day_Count has the language-defined range derived from Year_Number. Whole-day
addition/subtraction and Difference operate across the full calendar range,
including spans that cannot fit in Duration. Difference returns whole days
truncated toward zero and a remainder with the same sign and magnitude below
86400 seconds. Time-to-Time subtraction returning Day_Count uses that same day
count. Existing base Calendar subtraction returning Duration retains its
representability check and raises Time_Error on overflow.

Day_of_Week uses Calendar's UTC date and works before as well as after the epoch.
All Year/Month/Day/Hour/Minute/Second/Sub_Second accessors and all three Date Split
profiles are provided. Seconds_Of and the Day_Duration Split preserve Duration's
2**(-30) precision; the latter raises Time_Error for exactly 86400.0, a case left
unspecified by the original Ada 2005 wording. Sub_Second results stay below 1.0.
Public subtype violations raise Constraint_Error; invalid civil dates, offset
results outside the supported calendar and unrepresentable time arithmetic
raise Time_Error.

## Images and values

Date images use `YYYY-MM-DD HH:MM:SS` with optional `.dd`. Elapsed images use
`HH:MM:SS`, an optional minus sign and optional `.dd`. Fractions truncate to
hundredths as required. Date images take hundredths directly from Time's exact
microseconds, avoiding an extra Duration rounding at boundaries such as .01.
Elapsed images use the exact Duration ticks and safely handle Duration'First.

For elapsed magnitudes of 100 hours or more (implementation-defined by 9.6.1),
the hour field expands without leading zeros. Value accepts those expanded
images as well as the ordinary two-digit hour field. Separators, digit widths,
minute/second ranges and optional two-digit fractions are checked strictly;
Date Value also checks the civil date and supported range. Invalid strings and
values outside Duration/Time raise Constraint_Error. Arbitrary input bounds,
including strings ending at Integer'Last, work. Formatting is locale independent
and uses no shared result buffers.

The obsolescent `System.Tick` constant is now available with type Duration. It
represents Calendar's microsecond resolution rounded to 1074 Duration ticks;
it is not a scheduler latency guarantee. Existing delay semantics, including
protected-action rejection, remain in the [host contract](host-library.md).

## Validation and limits

`calendarchildren`, `calendarchecks` and `calendartasking` execute whole/per-unit
and pass deterministic emission checks. They cover all child-package profiles,
full-range signed differences, UTC/offset rollovers, fractional normalization,
strict invalid values, leap-second rejection, every hundredth-second boundary,
large and negative elapsed images, Duration extremes, non-one/high string bounds
and four tasks. Existing Calendar, host-directory and delay regressions remain.

`runtime.calendar_library` checks every one of the 182256 supported Gregorian
dates, weekday progression, every permitted offset, range/overflow cases,
negative epoch fractions and four concurrent C callers. Unchanged ACATS C96008A
and C96008B exercise base decomposition/named profiles/Clock/System.Tick and
midnight normalization alongside the existing C96006A/C96007A cases. The pinned
129-case backfill baseline remains separate.

Focused compiler and C-runtime ASan/UBSan checks and C-runtime TSan checks use
these paths. QBE-generated Ada accesses are not sanitizer-instrumented; host
LeakSanitizer remains unavailable. Native Windows execution and broader clock
portability remain part of Phase 8's host audit. No hard realtime or scheduler
latency guarantees are made.
