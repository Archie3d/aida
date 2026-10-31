with Ada.Calendar;
with Ada.Calendar.Arithmetic;
with Ada.Calendar.Formatting;
with Ada.Calendar.Time_Zones;
with Ada.Text_IO;
procedure CalendarChecks is
    package C renames Ada.Calendar;
    package A renames Ada.Calendar.Arithmetic;
    package F renames Ada.Calendar.Formatting;
    use type C.Time;
    use type Ada.Calendar.Time_Zones.Time_Offset;
    date : C.Time;
    span : Duration;
    y : C.Year_Number;
    m : C.Month_Number;
    d : C.Day_Number;
    h : F.Hour_Number;
    n : F.Minute_Number;
    s : F.Second_Number;
    sub : F.Second_Duration;
    procedure bad_date (text : String) is
    begin
        date := F.Value (Date => text);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end bad_date;
    procedure bad_span (text : String) is
    begin
        span := F.Value (Elapsed_Time => text);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end bad_span;
begin
    bad_date ("");
    bad_date ("2000-02-30 00:00:00");
    bad_date ("2100-02-29 00:00:00");
    bad_date ("2000-01-01 24:00:00");
    bad_date ("2000-01-01 00:60:00");
    bad_date ("2000-01-01 00:00:60");
    bad_date ("2000-01-01T00:00:00");
    bad_date ("2000-01-01 00:00:00.1");
    bad_date ("2000-01-01 00:00:00.000");
    bad_date ("2000-01-01 00:00:00.X0");
    bad_date (" 2000-01-01 00:00:00");
    bad_span ("");
    bad_span ("-");
    bad_span ("+00:00:00");
    bad_span ("0:00:00");
    bad_span ("00:60:00");
    bad_span ("00:00:60");
    bad_span ("00:00:00.1");
    bad_span ("00:00:00.000");
    bad_span ("99:00:00extra");
    bad_span ("99999999999999999:00:00");
    bad_span ("2386092:56:32");
    for failure in 1 .. 7 loop
        begin
            case failure is
                when 1 => date := F.Time_Of (2000, 1, 1, Leap_Second => True);
                when 2 => date := F.Time_Of (2000, 1, 1, 0, 0, 0, Leap_Second => True);
                when 3 => date := A."-" (C.Time_Of (1901, 1, 1), A.Day_Count (1));
                when 4 => date := A."+" (C.Time_Of (2399, 12, 31), A.Day_Count (1));
                when 5 => date := F.Time_Of (1901, 1, 1, Time_Zone => 60);
                when 6 => F.Split (C.Time_Of (1901, 1, 1), y, m, d, h, n, s, sub, Time_Zone => -60);
                when 7 => F.Split (86400.0, h, n, s, sub);
                when others => null;
            end case;
            raise Program_Error;
        exception
            when C.Time_Error => null;
        end;
    end loop;
    Ada.Text_IO.Put_Line ("calendarchecks: passed");
end CalendarChecks;
