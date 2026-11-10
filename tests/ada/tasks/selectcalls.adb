with Ada.Text_IO; use Ada.Text_IO;
with Ada.Real_Time;
with Ada.Calendar;
procedure SelectCalls is
    Evaluated : Natural := 0;
    X : Integer := 10;
    function Actual return Integer is
    begin Evaluated := Evaluated + 1; return 3; end Actual;
    function Timeout return Duration is
    begin
        if Evaluated /= 1 then raise Program_Error; end if;
        Evaluated := Evaluated + 1;
        return 0.0;
    end Timeout;
    protected Gate is
        entry Work (Input : Integer; Output : in out Integer);
        procedure Open;
        function Count return Natural;
    private Ready : Boolean := False;
    end Gate;
    protected body Gate is
        entry Work (Input : Integer; Output : in out Integer) when Ready is
        begin Output := Output + Input; end Work;
        procedure Open is begin Ready := True; end Open;
        function Count return Natural is begin return Work'Count; end Count;
    end Gate;
    task Server is
        entry Never (V : in out Integer);
        entry Await (V : in out Integer);
        entry Finish;
    end Server;
    task body Server is
    begin
        accept Await (V : in out Integer) do V := V + 2; end Await;
        accept Finish;
    end Server;
begin
    select
        Server.Never (X);
        raise Program_Error;
    else null;
    end select;
    select
        Gate.Work (Actual, X);
        raise Program_Error;
    or
        delay Timeout;
    end select;
    if Evaluated /= 2 or Gate.Count /= 0 or X /= 10 then raise Program_Error; end if;
    select Gate.Work (3, X); raise Program_Error;
    else null;
    end select;
    select Server.Never (X); raise Program_Error;
    or delay until Ada.Real_Time.Clock;
    end select;
    select Server.Never (X); raise Program_Error;
    or delay until Ada.Calendar.Clock;
    end select;
    Gate.Open;
    select Gate.Work (3, X);
    else raise Program_Error;
    end select;
    -- An open entry wins even when the timeout has already expired.
    select Gate.Work (4, X);
    or delay 0.0; raise Program_Error;
    end select;
    if X /= 17 then raise Program_Error; end if;
    select Server.Never (X); raise Program_Error;
    then abort null;
    end select;
    if X /= 17 then raise Program_Error; end if;
    loop
        select Server.Await (X); exit;
        else delay 0.0;
        end select;
    end loop;
    if X /= 19 then raise Program_Error; end if;
    Server.Finish;
    Put_Line ("entry call selects OK");
end SelectCalls;
