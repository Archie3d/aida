package body Ada.Calendar is
    kInvalid : constant Long_Integer := -9223372036854775807 - 1;
    function Clock_Value return Long_Integer;
    pragma Import (C, Clock_Value, "__ada_calendar_clock");
    function Compose (Year, Month, Day : Integer; Seconds : Duration) return Long_Integer;
    pragma Import (C, Compose, "__ada_calendar_compose");
    procedure Decompose (Date : Long_Integer; Year, Month, Day : out Integer;
                         Seconds : out Duration; Valid : out Boolean);
    pragma Import (C, Decompose, "__ada_calendar_split");
    function Add (Date : Long_Integer; Span : Duration; Subtract : Boolean) return Long_Integer;
    pragma Import (C, Add, "__ada_calendar_add");
    procedure Difference (Left, Right : Long_Integer; Span : out Duration; Valid : out Boolean);
    pragma Import (C, Difference, "__ada_calendar_difference");
    function Checked (Value : Long_Integer) return Time is
    begin
        if Value = kInvalid then
            raise Time_Error;
        end if;
        return Time (Value);
    end Checked;
    function Clock return Time is
    begin
        return Checked (Clock_Value);
    end Clock;
    function Time_Of (Year : Year_Number; Month : Month_Number; Day : Day_Number;
                      Seconds : Day_Duration := 0.0) return Time is
    begin
        return Checked (Compose (Year, Month, Day, Seconds));
    end Time_Of;
    procedure Split (Date : Time; Year : out Year_Number; Month : out Month_Number;
                     Day : out Day_Number; Seconds : out Day_Duration) is
        Y, M, D : Integer;
        S : Duration;
        Valid : Boolean;
    begin
        Decompose (Long_Integer (Date), Y, M, D, S, Valid);
        if not Valid then
            raise Time_Error;
        end if;
        Year := Y;
        Month := M;
        Day := D;
        Seconds := S;
    end Split;
    function Year (Date : Time) return Year_Number is
        Y : Year_Number;
        M : Month_Number;
        D : Day_Number;
        S : Day_Duration;
    begin
        Split (Date, Y, M, D, S);
        return Y;
    end Year;
    function Month (Date : Time) return Month_Number is
        Y : Year_Number;
        M : Month_Number;
        D : Day_Number;
        S : Day_Duration;
    begin
        Split (Date, Y, M, D, S);
        return M;
    end Month;
    function Day (Date : Time) return Day_Number is
        Y : Year_Number;
        M : Month_Number;
        D : Day_Number;
        S : Day_Duration;
    begin
        Split (Date, Y, M, D, S);
        return D;
    end Day;
    function Seconds (Date : Time) return Day_Duration is
        Y : Year_Number;
        M : Month_Number;
        D : Day_Number;
        S : Day_Duration;
    begin
        Split (Date, Y, M, D, S);
        return S;
    end Seconds;
    function "+" (Left : Time; Right : Duration) return Time is
    begin
        return Checked (Add (Long_Integer (Left), Right, False));
    end;
    function "+" (Left : Duration; Right : Time) return Time is
    begin
        return Right + Left;
    end;
    function "-" (Left : Time; Right : Duration) return Time is
    begin
        return Checked (Add (Long_Integer (Left), Right, True));
    end;
    function "-" (Left, Right : Time) return Duration is
        Span : Duration;
        Valid : Boolean;
    begin
        Difference (Long_Integer (Left), Long_Integer (Right), Span, Valid);
        if not Valid then
            raise Time_Error;
        end if;
        return Span;
    end;
    function "<" (Left, Right : Time) return Boolean is
    begin
        return Long_Integer (Left) < Long_Integer (Right);
    end;
    function "<=" (Left, Right : Time) return Boolean is
    begin
        return Long_Integer (Left) <= Long_Integer (Right);
    end;
    function ">" (Left, Right : Time) return Boolean is
    begin
        return Long_Integer (Left) > Long_Integer (Right);
    end;
    function ">=" (Left, Right : Time) return Boolean is
    begin
        return Long_Integer (Left) >= Long_Integer (Right);
    end;
end Ada.Calendar;
