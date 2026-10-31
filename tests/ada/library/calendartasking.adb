with Ada.Calendar;
with Ada.Calendar.Arithmetic;
with Ada.Calendar.Formatting;
with Ada.Calendar.Time_Zones;
with Ada.Text_IO;
procedure CalendarTasking is
    package C renames Ada.Calendar;
    package A renames Ada.Calendar.Arithmetic;
    package F renames Ada.Calendar.Formatting;
    package Z renames Ada.Calendar.Time_Zones;
    use type C.Time;
    use type A.Day_Count;
    use type Z.Time_Offset;
    protected results is
        procedure done;
        function count return Natural;
    private
        finished : Natural := 0;
    end results;
    protected body results is
        procedure done is
        begin
            finished := finished + 1;
        end done;
        function count return Natural is
        begin
            return finished;
        end count;
    end results;
    task type Worker;
    task body Worker is
        date : C.Time;
        days : A.Day_Count;
        seconds : Duration;
        leaps : A.Leap_Seconds_Count;
    begin
        for i in 1 .. 100 loop
            date := F.Value ("2000-02-29 12:34:56.25");
            if F.Image (date, True, -330) /= "2000-02-29 07:04:56.25"
                or Z.UTC_Time_Offset (C.Clock) /= 0 then
                raise Program_Error;
            end if;
            A.Difference (A."+" (date, A.Day_Count (50000)), date, days, seconds, leaps);
            if days /= 50000 or seconds /= 0.0 or leaps /= 0 then
                raise Program_Error;
            end if;
        end loop;
        results.done;
    end Worker;
begin
    declare
        workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    if results.count /= 4 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("calendartasking: passed");
end CalendarTasking;
