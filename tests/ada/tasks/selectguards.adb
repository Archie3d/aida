with Ada.Text_IO; use Ada.Text_IO;
with Ada.Real_Time;
with Ada.Calendar;
procedure SelectGuards is
    Evaluated : Natural := 0;
    function Bad_Index return Integer is
    begin raise Program_Error; return 1; end Bad_Index;
    function Bad_Delay return Duration is
    begin raise Program_Error; return 0.0; end Bad_Delay;
    task Server is entry Work (1 .. 2) (V : in out Integer); end Server;
    task body Server is
        function Guard return Boolean is
        begin Evaluated := Evaluated + 1; return False; end Guard;
    begin
        select
            when Guard => accept Work (Bad_Index) (V : in out Integer);
        else
            null;
        end select;
        select
            when False => accept Work (Bad_Index) (V : in out Integer);
        or
            when False => delay Bad_Delay;
        or
            delay 0.0;
        end select;
        begin
            select
                when False => accept Work (Bad_Index) (V : in out Integer);
            end select;
            raise Constraint_Error;
        exception when Program_Error => null;
        end;
        select
            accept Work (1) (V : in out Integer) do V := V + 2; end Work;
        or
            delay 2.0;
            raise Program_Error;
        or
            delay 3.0;
            raise Program_Error;
        end select;
        select
            when False => accept Work (2) (V : in out Integer);
        or
            delay until Ada.Real_Time.Clock;
        end select;
        select
            when False => accept Work (2) (V : in out Integer);
        or
            delay until Ada.Calendar.Clock;
        end select;
        loop
            select
                accept Work (2) (V : in out Integer) do V := V + 3; end Work;
            or
                terminate;
            end select;
        end loop;
    end Server;
    task type Idle is entry Work; end Idle;
    task body Idle is
    begin
        loop
            select accept Work;
            or terminate;
            end select;
        end loop;
    end Idle;
    A, B : Idle;
    X : Integer := 10;
begin
    Server.Work (1) (X);
    Server.Work (2) (X);
    if X /= 15 or Evaluated /= 1 then raise Program_Error; end if;
    Put_Line ("selective accept OK");
end SelectGuards;
