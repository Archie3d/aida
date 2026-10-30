package body Ada.Calendar.Formatting is
    function Weekday (Date : Long_Integer) return Integer;
    pragma Import (C, Weekday, "__ada_calendar_weekday");
    function Centiseconds (Date : Long_Integer) return Integer;
    pragma Import (C, Centiseconds, "__ada_calendar_centiseconds");
    function Compose (Year, Month, Day : Integer; Seconds : Duration;
                       Offset : Integer) return Long_Integer;
    pragma Import (C, Compose, "__ada_calendar_compose_zone");
    procedure Decompose (Date : Long_Integer; Offset : Integer;
                         Year, Month, Day : out Integer; Seconds : out Duration; Valid : out Boolean);
    pragma Import (C, Decompose, "__ada_calendar_split_zone");
    procedure Elapsed_Parts (Span : Duration; Hours, Minutes, Seconds, Hundredths : out Integer;
                             Negative : out Boolean);
    pragma Import (C, Elapsed_Parts, "__ada_calendar_elapsed_parts");

    function Day_of_Week (Date : Time) return Day_Name is
        day : Integer := Weekday (Long_Integer (Date));
    begin
        if day < 0 then
            raise Time_Error;
        end if;
        return Day_Name'Val (day);
    end Day_of_Week;

    function Seconds_Of (Hour : Hour_Number; Minute : Minute_Number;
                         Second : Second_Number := 0;
                         Sub_Second : Second_Duration := 0.0) return Day_Duration is
    begin
        return Duration (Hour * 3600 + Minute * 60 + Second) + Sub_Second;
    end Seconds_Of;

    procedure Split (Seconds : Day_Duration; Hour : out Hour_Number;
                     Minute : out Minute_Number; Second : out Second_Number;
                     Sub_Second : out Second_Duration) is
        whole : Integer := Integer (Seconds);
    begin
        -- Ada integer conversion rounds. Adjust to the preceding whole second.
        if Seconds = 86400.0 then
            raise Time_Error;
        end if;
        if Duration (whole) > Seconds then
            whole := whole - 1;
        end if;
        Hour := whole / 3600;
        Minute := (whole / 60) mod 60;
        Second := whole mod 60;
        Sub_Second := Seconds - Duration (whole);
    end Split;

    function Time_Of (Year : Year_Number; Month : Month_Number; Day : Day_Number;
                      Seconds : Day_Duration := 0.0; Leap_Second : Boolean := False;
                      Time_Zone : Time_Zones.Time_Offset := 0) return Time is
        value : Long_Integer;
    begin
        if Leap_Second then
            raise Time_Error;
        end if;
        value := Compose (Year, Month, Day, Seconds, Integer (Time_Zone));
        if value = Long_Integer'First then
            raise Time_Error;
        end if;
        return Time (value);
    end Time_Of;

    function Time_Of (Year : Year_Number; Month : Month_Number; Day : Day_Number;
                      Hour : Hour_Number; Minute : Minute_Number; Second : Second_Number;
                      Sub_Second : Second_Duration := 0.0; Leap_Second : Boolean := False;
                      Time_Zone : Time_Zones.Time_Offset := 0) return Time is
    begin
        return Time_Of (Year, Month, Day, Seconds_Of (Hour, Minute, Second, Sub_Second),
                        Leap_Second, Time_Zone);
    end Time_Of;

    procedure Split (Date : Time; Year : out Year_Number; Month : out Month_Number;
                     Day : out Day_Number; Seconds : out Day_Duration;
                     Leap_Second : out Boolean; Time_Zone : Time_Zones.Time_Offset := 0) is
        y, m, d : Integer;
        s : Duration;
        valid : Boolean;
    begin
        Decompose (Long_Integer (Date), Integer (Time_Zone), y, m, d, s, valid);
        if not valid then
            raise Time_Error;
        end if;
        Year := y;
        Month := m;
        Day := d;
        Seconds := s;
        Leap_Second := False;
    end Split;

    procedure Split (Date : Time; Year : out Year_Number; Month : out Month_Number;
                     Day : out Day_Number; Hour : out Hour_Number; Minute : out Minute_Number;
                     Second : out Second_Number; Sub_Second : out Second_Duration;
                     Leap_Second : out Boolean; Time_Zone : Time_Zones.Time_Offset := 0) is
        seconds : Day_Duration;
    begin
        Split (Date, Year, Month, Day, seconds, Leap_Second, Time_Zone);
        Split (seconds, Hour, Minute, Second, Sub_Second);
    end Split;

    procedure Split (Date : Time; Year : out Year_Number; Month : out Month_Number;
                     Day : out Day_Number; Hour : out Hour_Number; Minute : out Minute_Number;
                     Second : out Second_Number; Sub_Second : out Second_Duration;
                     Time_Zone : Time_Zones.Time_Offset := 0) is
        leap : Boolean;
    begin
        Split (Date, Year, Month, Day, Hour, Minute, Second, Sub_Second, leap, Time_Zone);
    end Split;

    function Year (Date : Time; Time_Zone : Time_Zones.Time_Offset := 0) return Year_Number is
        y : Year_Number;
        m : Month_Number;
        d : Day_Number;
        h : Hour_Number;
        n : Minute_Number;
        s : Second_Number;
        sub : Second_Duration;
    begin
        Split (Date, y, m, d, h, n, s, sub, Time_Zone);
        return y;
    end Year;

    function Month (Date : Time; Time_Zone : Time_Zones.Time_Offset := 0) return Month_Number is
        y : Year_Number;
        m : Month_Number;
        d : Day_Number;
        h : Hour_Number;
        n : Minute_Number;
        s : Second_Number;
        sub : Second_Duration;
    begin
        Split (Date, y, m, d, h, n, s, sub, Time_Zone);
        return m;
    end Month;

    function Day (Date : Time; Time_Zone : Time_Zones.Time_Offset := 0) return Day_Number is
        y : Year_Number;
        m : Month_Number;
        d : Day_Number;
        h : Hour_Number;
        n : Minute_Number;
        s : Second_Number;
        sub : Second_Duration;
    begin
        Split (Date, y, m, d, h, n, s, sub, Time_Zone);
        return d;
    end Day;

    function Hour (Date : Time; Time_Zone : Time_Zones.Time_Offset := 0) return Hour_Number is
        y : Year_Number;
        m : Month_Number;
        d : Day_Number;
        h : Hour_Number;
        n : Minute_Number;
        s : Second_Number;
        sub : Second_Duration;
    begin
        Split (Date, y, m, d, h, n, s, sub, Time_Zone);
        return h;
    end Hour;

    function Minute (Date : Time; Time_Zone : Time_Zones.Time_Offset := 0) return Minute_Number is
        y : Year_Number;
        m : Month_Number;
        d : Day_Number;
        h : Hour_Number;
        n : Minute_Number;
        s : Second_Number;
        sub : Second_Duration;
    begin
        Split (Date, y, m, d, h, n, s, sub, Time_Zone);
        return n;
    end Minute;

    function Second (Date : Time) return Second_Number is
        y : Year_Number;
        m : Month_Number;
        d : Day_Number;
        h : Hour_Number;
        n : Minute_Number;
        s : Second_Number;
        sub : Second_Duration;
    begin
        Split (Date, y, m, d, h, n, s, sub, 0);
        return s;
    end Second;

    function Sub_Second (Date : Time) return Second_Duration is
        y : Year_Number;
        m : Month_Number;
        d : Day_Number;
        h : Hour_Number;
        n : Minute_Number;
        s : Second_Number;
        sub : Second_Duration;
    begin
        Split (Date, y, m, d, h, n, s, sub, 0);
        return sub;
    end Sub_Second;

    procedure Put_Digits (Target : in out String; Offset, Width, Value : Natural) is
        rest : Natural := Value;
    begin
        for i in reverse 0 .. Width - 1 loop
            Target (Target'First + (Offset + i)) := Character'Val (Character'Pos ('0') + rest mod 10);
            rest := rest / 10;
        end loop;
    end Put_Digits;

    function Digits_Value (Source : String; Offset, Width : Natural) return Natural is
        result : Natural := 0;
        digit : Natural;
        c : Character;
    begin
        for i in 0 .. Width - 1 loop
            c := Source (Source'First + (Offset + i));
            if c < '0' or c > '9' then
                raise Constraint_Error;
            end if;
            digit := Character'Pos (c) - Character'Pos ('0');
            if result > (Natural'Last - digit) / 10 then
                raise Constraint_Error;
            end if;
            result := result * 10 + digit;
        end loop;
        return result;
    end Digits_Value;

    function Image (Date : Time; Include_Time_Fraction : Boolean := False;
                    Time_Zone : Time_Zones.Time_Offset := 0) return String is
        result : String := "0000-00-00 00:00:00.00";
        y : Year_Number;
        m : Month_Number;
        d : Day_Number;
        h : Hour_Number;
        n : Minute_Number;
        s : Second_Number;
        sub : Second_Duration;
    begin
        Split (Date, y, m, d, h, n, s, sub, Time_Zone);
        Put_Digits (result, 0, 4, y);
        Put_Digits (result, 5, 2, m);
        Put_Digits (result, 8, 2, d);
        Put_Digits (result, 11, 2, h);
        Put_Digits (result, 14, 2, n);
        Put_Digits (result, 17, 2, s);
        -- Work in Time's exact microseconds, avoiding a second rounding via
        -- Duration at hundredth-second boundaries.
        Put_Digits (result, 20, 2, Centiseconds (Long_Integer (Date)));
        if Include_Time_Fraction then
            return result;
        end if;
        return result (1 .. 19);
    end Image;

    function Value (Date : String; Time_Zone : Time_Zones.Time_Offset := 0) return Time is
        fraction : Second_Duration := 0.0;
    begin
        if Date'Length /= 19 and Date'Length /= 22 then
            raise Constraint_Error;
        end if;
        if Date (Date'First + 4) /= '-' or Date (Date'First + 7) /= '-'
            or Date (Date'First + 10) /= ' ' or Date (Date'First + 13) /= ':'
            or Date (Date'First + 16) /= ':' then
            raise Constraint_Error;
        end if;
        if Date'Length = 22 then
            if Date (Date'First + 19) /= '.' then
                raise Constraint_Error;
            end if;
            fraction := Duration (Digits_Value (Date, 20, 2)) / 100;
        end if;
        return Time_Of (Year_Number (Digits_Value (Date, 0, 4)),
                        Month_Number (Digits_Value (Date, 5, 2)),
                        Day_Number (Digits_Value (Date, 8, 2)),
                        Hour_Number (Digits_Value (Date, 11, 2)),
                        Minute_Number (Digits_Value (Date, 14, 2)),
                        Second_Number (Digits_Value (Date, 17, 2)),
                        fraction, False, Time_Zone);
    exception
        when Time_Error => raise Constraint_Error;
    end Value;

    function Image (Elapsed_Time : Duration; Include_Time_Fraction : Boolean := False) return String is
        h, m, s, centi : Integer;
        negative : Boolean;
        suffix : String := ":00:00.00";
    begin
        Elapsed_Parts (Elapsed_Time, h, m, s, centi, negative);
        Put_Digits (suffix, 1, 2, m);
        Put_Digits (suffix, 4, 2, s);
        Put_Digits (suffix, 7, 2, centi);
        declare
            hourText : String := Integer'Image (h);
            hourDigits : String := hourText (2 .. hourText'Last);
        begin
            if Include_Time_Fraction then
                if h < 10 then
                    if negative then return "-0" & hourDigits & suffix; end if;
                    return "0" & hourDigits & suffix;
                end if;
                if negative then return "-" & hourDigits & suffix; end if;
                return hourDigits & suffix;
            end if;
            if h < 10 then
                if negative then return "-0" & hourDigits & suffix (1 .. 6); end if;
                return "0" & hourDigits & suffix (1 .. 6);
            end if;
            if negative then return "-" & hourDigits & suffix (1 .. 6); end if;
            return hourDigits & suffix (1 .. 6);
        end;
    end Image;

    function Value (Elapsed_Time : String) return Duration is
        length : Natural := Elapsed_Time'Length;
        offset : Natural := 0;
        hours, minutes, seconds : Natural;
        fraction : Second_Duration := 0.0;
        whole : Long_Integer;
        negative : Boolean := False;
    begin
        if length = 0 then raise Constraint_Error; end if;
        if Elapsed_Time (Elapsed_Time'First) = '-' then
            negative := True;
            offset := 1;
        end if;
        if length - offset < 8 then raise Constraint_Error; end if;
        declare
            hourWidth : Natural := length - offset - 6;
        begin
            if Elapsed_Time (Elapsed_Time'Last - 2) = '.' then
                hourWidth := hourWidth - 3;
                fraction := Duration (Digits_Value (Elapsed_Time, length - 2, 2)) / 100;
            end if;
            if hourWidth < 2 or Elapsed_Time (Elapsed_Time'First + (offset + hourWidth)) /= ':'
                or Elapsed_Time (Elapsed_Time'First + (offset + hourWidth + 3)) /= ':' then
                raise Constraint_Error;
            end if;
            hours := Digits_Value (Elapsed_Time, offset, hourWidth);
            if hourWidth > 2 and Elapsed_Time (Elapsed_Time'First + offset) = '0' then
                raise Constraint_Error;
            end if;
            minutes := Digits_Value (Elapsed_Time, offset + hourWidth + 1, 2);
            seconds := Digits_Value (Elapsed_Time, offset + hourWidth + 4, 2);
            if minutes > 59 or seconds > 59 then raise Constraint_Error; end if;
            whole := Long_Integer (hours) * 3600 + Long_Integer (minutes) * 60 + Long_Integer (seconds);
        end;
        if negative then return Duration (-whole) - fraction; end if;
        return Duration (whole) + fraction;
    end Value;
end Ada.Calendar.Formatting;
