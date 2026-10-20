-- Signed 64-bit nanoseconds on CLOCK_MONOTONIC's unspecified epoch.
-- Tick is the host's reported monotonic resolution, rounded up to 1 ns.
package Ada.Real_Time is
    type Time is private;
    Time_First : constant Time;
    Time_Last : constant Time;
    Time_Unit : constant := 0.000_000_001;

    type Time_Span is private;
    Time_Span_First : constant Time_Span;
    Time_Span_Last : constant Time_Span;
    Time_Span_Zero : constant Time_Span;
    Time_Span_Unit : constant Time_Span;
    Tick : constant Time_Span;

    function Clock return Time;
    pragma Import (C, Clock, "__ada_real_clock");

    function "+" (left : Time; right : Time_Span) return Time;
    function "+" (left : Time_Span; right : Time) return Time;
    function "-" (left : Time; right : Time_Span) return Time;
    function "-" (left, right : Time) return Time_Span;
    function "<" (left, right : Time) return Boolean;
    function "<=" (left, right : Time) return Boolean;
    function ">" (left, right : Time) return Boolean;
    function ">=" (left, right : Time) return Boolean;
    function "<" (left, right : Time_Span) return Boolean;
    function "<=" (left, right : Time_Span) return Boolean;
    function ">" (left, right : Time_Span) return Boolean;
    function ">=" (left, right : Time_Span) return Boolean;
    function "+" (left, right : Time_Span) return Time_Span;
    function "-" (left, right : Time_Span) return Time_Span;
    function "-" (right : Time_Span) return Time_Span;
    function "*" (left : Time_Span; right : Integer) return Time_Span;
    function "*" (left : Integer; right : Time_Span) return Time_Span;
    function "/" (left, right : Time_Span) return Integer;
    function "/" (left : Time_Span; right : Integer) return Time_Span;
    function "abs" (right : Time_Span) return Time_Span;
    function Nanoseconds (ns : Integer) return Time_Span;
    function Microseconds (us : Integer) return Time_Span;
    function Milliseconds (ms : Integer) return Time_Span;
    function Seconds (s : Integer) return Time_Span;
    function Minutes (m : Integer) return Time_Span;

    function To_Duration (ts : Time_Span) return Duration;
    function To_Time_Span (d : Duration) return Time_Span;

    type Seconds_Count is range -9223372036854775807 - 1 .. 9223372036854775807;
    procedure Split (t : Time; sc : out Seconds_Count; ts : out Time_Span);
    function Time_Of (sc : Seconds_Count; ts : Time_Span) return Time;
private
    type Time is range -9223372036854775807 - 1 .. 9223372036854775807;
    type Time_Span is range -9223372036854775807 - 1 .. 9223372036854775807;
    Time_First : constant Time := Time'First;
    Time_Last : constant Time := Time'Last;
    Time_Span_First : constant Time_Span := Time_Span'First;
    Time_Span_Last : constant Time_Span := Time_Span'Last;
    Time_Span_Zero : constant Time_Span := 0;
    Time_Span_Unit : constant Time_Span := 1;
    function Clock_Resolution return Time_Span;
    pragma Import (C, Clock_Resolution, "__ada_real_tick");
    Tick : constant Time_Span := Clock_Resolution;
end Ada.Real_Time;
