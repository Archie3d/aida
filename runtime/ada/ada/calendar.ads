-- UTC civil time, microseconds since the Unix epoch; no leap seconds.
package Ada.Calendar is
    type Time is private;
    subtype Year_Number is Integer range 1901 .. 2399;
    subtype Month_Number is Integer range 1 .. 12;
    subtype Day_Number is Integer range 1 .. 31;
    subtype Day_Duration is Duration range 0.0 .. 86400.0;
    function Clock return Time;
    function Year (Date : Time) return Year_Number;
    function Month (Date : Time) return Month_Number;
    function Day (Date : Time) return Day_Number;
    function Seconds (Date : Time) return Day_Duration;
    procedure Split (Date : Time; Year : out Year_Number; Month : out Month_Number;
                     Day : out Day_Number; Seconds : out Day_Duration);
    function Time_Of (Year : Year_Number; Month : Month_Number; Day : Day_Number;
                      Seconds : Day_Duration := 0.0) return Time;
    function "+" (Left : Time; Right : Duration) return Time;
    function "+" (Left : Duration; Right : Time) return Time;
    function "-" (Left : Time; Right : Duration) return Time;
    function "-" (Left, Right : Time) return Duration;
    function "<" (Left, Right : Time) return Boolean;
    function "<=" (Left, Right : Time) return Boolean;
    function ">" (Left, Right : Time) return Boolean;
    function ">=" (Left, Right : Time) return Boolean;
    Time_Error : exception;
private
    type Time is range -9223372036854775807 - 1 .. 9223372036854775807;
end Ada.Calendar;
