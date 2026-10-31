with Ada.Real_Time; use Ada.Real_Time;
with Ada.Text_IO;
with Ada.Text_IO.Fixed_IO;
procedure TimeValues is
    package Duration_IO is new Ada.Text_IO.Fixed_IO (Duration);
    image : String (1 .. 16);
    span : Time_Span := Nanoseconds (1);
    instant : Time := Time_Of (0, Time_Span_Zero);
    sc : Seconds_Count;
    remainder : Time_Span;
    d : Duration := 1.5;
    quotient : Integer;
    failures : Integer := 0;
begin
    if Duration'Small /= 2.0 ** (-30) or Duration'Delta /= Duration'Small
        or Duration'Size /= 64 or Duration'First > -86400.0 or Duration'Last < 86400.0 then
        raise Program_Error;
    end if;
    d := d * 2;
    if d /= 3.0 or d / 2 /= 1.5 then
        raise Program_Error;
    end if;
    Duration_IO.Put (image, d, Aft => 2, Exp => 0);
    if image /= "            3.00" then
        raise Program_Error;
    end if;
    if Time_Unit /= 0.000_000_001 or Tick < Time_Span_Unit then
        raise Program_Error;
    end if;
    if Nanoseconds (1000) /= Microseconds (1) or Microseconds (1000) /= Milliseconds (1)
        or Milliseconds (1000) /= Seconds (1) or Seconds (60) /= Minutes (1)
        or Seconds (-1) /= -Seconds (1) then
        raise Program_Error;
    end if;
    if Seconds (2) / Seconds (1) /= 2 or Seconds (3) / 2 /= Milliseconds (1500)
        or 2 * Seconds (1) /= Seconds (1) * 2 or abs Seconds (-1) /= Seconds (1)
        or Seconds (1) + Seconds (2) - Seconds (1) /= Seconds (2) then
        raise Program_Error;
    end if;
    if To_Duration (Time_Span_Zero) /= 0.0 or To_Time_Span (0.0) /= Time_Span_Zero
        or To_Duration (Seconds (3)) /= 3.0 or To_Time_Span (1.5) /= Milliseconds (1500)
        or To_Time_Span (Duration'Small * 1048576) /= Nanoseconds (976563)
        or To_Time_Span (-Duration'Small * 1048576) /= Nanoseconds (-976563) then
        raise Program_Error;
    end if;
    instant := Time_Of (-2, Milliseconds (750));
    Split (instant, sc, remainder);
    if sc /= -2 or remainder /= Milliseconds (750) or Time_Of (sc, remainder) /= instant then
        raise Program_Error;
    end if;
    Split (Time_First, sc, remainder);
    if sc /= -9223372037 or remainder /= Nanoseconds (145224192)
        or Time_Of (sc, remainder) /= Time_First then
        raise Program_Error;
    end if;
    Split (Time_Last, sc, remainder);
    if sc /= 9223372036 or remainder /= Nanoseconds (854775807)
        or Time_Of (9223372037, Nanoseconds (-145224193)) /= Time_Last then
        raise Program_Error;
    end if;
    instant := Time_Of (1, Nanoseconds (-1));
    if instant - Time_Of (0, Time_Span_Zero) /= Seconds (1) - Time_Span_Unit
        or instant + span /= span + instant or instant - span >= instant
        or instant <= Time_First or Time_Last < instant then
        raise Program_Error;
    end if;
    begin
        span := Time_Span_Last + Time_Span_Unit;
    exception
        when Constraint_Error => failures := failures + 1;
    end;
    begin
        instant := Time_Last + Time_Span_Unit;
    exception
        when Constraint_Error => failures := failures + 1;
    end;
    begin
        instant := Time_First - Time_Span_Unit;
    exception
        when Constraint_Error => failures := failures + 1;
    end;
    begin
        span := -Time_Span_First;
    exception
        when Constraint_Error => failures := failures + 1;
    end;
    begin
        span := abs Time_Span_First;
    exception
        when Constraint_Error => failures := failures + 1;
    end;
    begin
        quotient := Time_Span_Last / Time_Span_Unit;
    exception
        when Constraint_Error => failures := failures + 1;
    end;
    begin
        span := Seconds (1) / 0;
    exception
        when Constraint_Error => failures := failures + 1;
    end;
    begin
        span := Time_Span_First / (-1);
    exception
        when Constraint_Error => failures := failures + 1;
    end;
    begin
        span := Time_Span_Last * 2;
    exception
        when Constraint_Error => failures := failures + 1;
    end;
    begin
        span := Minutes (Integer'Last);
    exception
        when Constraint_Error => failures := failures + 1;
    end;
    begin
        d := To_Duration (Time_Span_Last);
    exception
        when Constraint_Error => failures := failures + 1;
    end;
    begin
        instant := Time_Of (Seconds_Count'Last, Time_Span_Zero);
    exception
        when Constraint_Error => failures := failures + 1;
    end;
    declare
        smallSeconds : Seconds_Count range 0 .. 0 := 0;
    begin
        Split (Time_Of (1, Time_Span_Zero), smallSeconds, remainder);
    exception
        when Constraint_Error => failures := failures + 1;
    end;
    if failures /= 13 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("time values passed");
end TimeValues;
