with Ada.Real_Time; use Ada.Real_Time;
with Ada.Text_IO;
procedure TimeDelays is
    start : Time := Clock;
    interval : Duration := 0.002;
    calls : Integer := 0;
    function Next_Instant return Time is
    begin
        calls := calls + 1;
        return Clock + Milliseconds (2);
    end Next_Instant;
    function Next_Interval return Duration is
    begin
        calls := calls + 1;
        return interval;
    end Next_Interval;
    protected Counter is
        procedure Add;
        procedure Wait_Relative;
        procedure Wait_Absolute;
        function Get return Integer;
    private
        total : Integer := 0;
    end Counter;
    protected body Counter is
        procedure Add is
        begin
            total := total + 1;
        end Add;
        procedure Wait_Relative is
            procedure Nested is
            begin
                delay 0.0;
            end Nested;
        begin
            Nested;
        end Wait_Relative;
        procedure Wait_Absolute is
        begin
            delay until Time_First;
        end Wait_Absolute;
        function Get return Integer is
        begin
            return total;
        end Get;
    end Counter;
    generic
        period : Duration;
    procedure Pause;
    procedure Pause is
    begin
        delay period;
    end Pause;
    procedure Brief is new Pause (0.001);
begin
    delay Next_Interval;
    if Clock - start < To_Time_Span (interval) then
        raise Program_Error;
    end if;
    delay until Next_Instant;
    if calls /= 2 then
        raise Program_Error;
    end if;
    delay 0.0;
    delay -1.0;
    delay until Time_First;
    delay until Clock - Seconds (1);
    Brief;
    begin
        Counter.Wait_Relative;
        raise Constraint_Error;
    exception
        when Program_Error => null;
    end;
    begin
        Counter.Wait_Absolute;
        raise Constraint_Error;
    exception
        when Program_Error => null;
    end;
    Counter.Add;
    declare
        task type Periodic;
        task body Periodic is
            next : Time := Clock;
            period : Time_Span := Milliseconds (1);
        begin
            for iteration in 1 .. 5 loop
                next := next + period;
                delay until next;
                if Clock < next then
                    raise Program_Error;
                end if;
                Counter.Add;
            end loop;
        end Periodic;
        first, second : Periodic;
    begin
        null;
    end;
    if Counter.Get /= 11 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("time delays passed");
end TimeDelays;
