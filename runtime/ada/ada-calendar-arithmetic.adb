package body Ada.Calendar.Arithmetic is
    function Add_Days (Date, Days : Long_Integer; Subtract : Boolean) return Long_Integer;
    pragma Import (C, Add_Days, "__ada_calendar_add_days");
    procedure Day_Difference (Left, Right : Long_Integer; Days : out Integer;
                              Seconds : out Duration; Valid : out Boolean);
    pragma Import (C, Day_Difference, "__ada_calendar_day_difference");
    function Checked (Value : Long_Integer) return Time is
    begin
        if Value = Long_Integer'First then
            raise Time_Error;
        end if;
        return Time (Value);
    end Checked;
    procedure Difference (Left, Right : Time; Days : out Day_Count;
                          Seconds : out Duration; Leap_Seconds : out Leap_Seconds_Count) is
        whole : Integer;
        rest : Duration;
        valid : Boolean;
    begin
        Day_Difference (Long_Integer (Left), Long_Integer (Right), whole, rest, valid);
        if not valid then
            raise Time_Error;
        end if;
        Days := Day_Count (whole);
        Seconds := rest;
        Leap_Seconds := 0;
    end Difference;
    function "+" (Left : Time; Right : Day_Count) return Time is
    begin
        return Checked (Add_Days (Long_Integer (Left), Long_Integer (Right), False));
    end "+";
    function "+" (Left : Day_Count; Right : Time) return Time is
    begin
        return Right + Left;
    end "+";
    function "-" (Left : Time; Right : Day_Count) return Time is
    begin
        return Checked (Add_Days (Long_Integer (Left), Long_Integer (Right), True));
    end "-";
    function "-" (Left, Right : Time) return Day_Count is
        days : Day_Count;
        seconds : Duration;
        leaps : Leap_Seconds_Count;
    begin
        Difference (Left, Right, days, seconds, leaps);
        return days;
    end "-";
end Ada.Calendar.Arithmetic;
