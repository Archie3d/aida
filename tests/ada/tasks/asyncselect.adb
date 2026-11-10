with Ada.Text_IO; use Ada.Text_IO;
with Ada.Real_Time;
with Ada.Calendar;
procedure AsyncSelect is
    X : Integer := 0;
    protected Gate is
        entry Work (V : in out Integer);
        entry Fail (V : in out Integer);
        procedure Open;
        function Count return Natural;
    private
        Ready : Boolean := False;
    end Gate;
    protected body Gate is
        entry Work (V : in out Integer) when Ready is
        begin V := V + 4; end Work;
        entry Fail (V : in out Integer) when Ready is
        begin V := 999; raise Constraint_Error; end Fail;
        procedure Open is begin Ready := True; end Open;
        function Count return Natural is begin return Work'Count; end Count;
    end Gate;
    task Server is
        entry Work (V : in out Integer);
        entry Fail (V : in out Integer);
    end Server;
    task body Server is
    begin
        delay 0.02;
        accept Work (V : in out Integer) do
            select
                delay 0.001;
                V := V + 3;
            then abort
                delay 2.0;
                raise Program_Error;
            end select;
        end Work;
        accept Fail (V : in out Integer) do
            V := 999;
            raise Constraint_Error;
        end Fail;
    exception when Constraint_Error => null;
    end Server;
begin
    select
        delay 0.0;
        X := 1;
    then abort
        raise Program_Error;
    end select;
    select
        delay until Ada.Real_Time.Clock;
    then abort
        raise Program_Error;
    end select;
    select
        delay until Ada.Calendar.Clock;
    then abort
        raise Program_Error;
    end select;
    select
        delay 0.001;
        X := X + 1;
    then abort
        begin
            loop null; end loop;
        exception when others => raise Program_Error;
        end;
    end select;
    select
        Server.Work (X);
        if X /= 5 then raise Program_Error; end if;
    then abort
        delay 2.0;
        raise Program_Error;
    end select;
    select
        Gate.Work (X);
        raise Program_Error;
    then abort
        if Gate.Count /= 1 then raise Program_Error; end if;
    end select;
    if Gate.Count /= 0 or X /= 5 then raise Program_Error; end if;
    select
        Gate.Work (X);
        if X /= 9 then raise Program_Error; end if;
    then abort
        Gate.Open;
        delay 2.0;
        raise Program_Error;
    end select;
    select
        Gate.Work (X);
    then abort
        raise Program_Error;
    end select;
    if X /= 13 then raise Program_Error; end if;
    -- Completing the part cancels a delay, including an enclosing trigger.
    select
        delay 10.0;
        raise Program_Error;
    then abort
        select
            delay 0.001;
        then abort
            delay 2.0;
        end select;
    end select;
    -- An outer trigger unwinds and cancels the inner operation.
    select
        delay 0.001;
        X := X + 1;
    then abort
        select
            delay 10.0;
            raise Program_Error;
        then abort
            delay 2.0;
        end select;
        raise Program_Error;
    end select;
    if X /= 14 then raise Program_Error; end if;
    begin
        select Server.Fail (X); raise Program_Error;
        then abort delay 2.0; raise Program_Error;
        end select;
    exception when Constraint_Error => null;
    end;
    if X /= 14 then raise Program_Error; end if;
    begin
        select Gate.Fail (X); raise Program_Error;
        then abort raise Program_Error;
        end select;
    exception when Constraint_Error => null;
    end;
    if X /= 14 then raise Program_Error; end if;
    Put_Line ("asynchronous select OK");
end AsyncSelect;
