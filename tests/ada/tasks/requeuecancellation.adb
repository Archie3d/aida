with Ada.Text_IO; use Ada.Text_IO;
with Ada.Task_Identification; use Ada.Task_Identification;
procedure RequeueCancellation is
    X : Integer := 3;
    Shared : Integer := 10;
    Command : Natural := 0;
    Bad : Boolean := False;
    pragma Atomic (Shared);
    pragma Atomic (Command);
    pragma Atomic (Bad);
    protected Gate is
        entry Safe (N : in out Integer);
        entry Unsafe (N : in out Integer);
        entry Wait (M : in out Integer);
        procedure Open;
        procedure Close;
        function Count return Natural;
    private Ready : Boolean := False;
    end Gate;
    protected body Gate is
        entry Safe (N : in out Integer) when True is
        begin N := N + 1; requeue Wait with abort; end Safe;
        entry Unsafe (N : in out Integer) when True is
        begin N := N + 1; requeue Wait; end Unsafe;
        entry Wait (M : in out Integer) when Ready is
        begin M := M + 2; end Wait;
        procedure Open is begin Ready := True; end Open;
        procedure Close is begin Ready := False; end Close;
        function Count return Natural is begin return Wait'Count; end Count;
    end Gate;
    task Opener;
    task body Opener is
    begin
        for Round in 1 .. 2 loop
            while Command /= Round loop delay 0.0; end loop;
            while Gate.Count = 0 loop delay 0.0; end loop;
            Gate.Open;
        end loop;
    end Opener;
    task Abortable_Caller is entry Run; end Abortable_Caller;
    task body Abortable_Caller is
    begin
        accept Run;
        Gate.Safe (Shared);
        Bad := True;
    exception when others => Bad := True;
    end Abortable_Caller;
    task Protected_Caller is entry Run; end Protected_Caller;
    task body Protected_Caller is
    begin
        accept Run;
        Gate.Unsafe (Shared);
        Bad := True;
    exception when others => Bad := True;
    end Protected_Caller;
begin
    -- A plain requeue remains protected against an already expired timeout.
    Command := 1;
    select Gate.Unsafe (X);
    or delay 0.0; raise Program_Error;
    end select;
    if X /= 6 then raise Program_Error; end if;
    Gate.Close;
    -- Immediate selection followed only by plain requeues skips the part.
    Command := 2;
    select Gate.Unsafe (X);
    then abort raise Program_Error;
    end select;
    if X /= 9 then raise Program_Error; end if;
    Gate.Close;
    select Gate.Safe (X); raise Program_Error;
    or delay 0.0; null;
    end select;
    if X /= 9 or Gate.Count /= 0 then raise Program_Error; end if;
    select Gate.Safe (X); raise Program_Error;
    else null;
    end select;
    if X /= 9 or Gate.Count /= 0 then raise Program_Error; end if;
    -- Requeue-with-abort enables the part even after immediate selection.
    select Gate.Safe (X);
    then abort
        Gate.Open;
        delay 2.0;
        raise Program_Error;
    end select;
    if X /= 12 then raise Program_Error; end if;
    Gate.Close;
    select Gate.Safe (X); raise Program_Error;
    then abort null;
    end select;
    if X /= 12 or Gate.Count /= 0 then raise Program_Error; end if;
    Abortable_Caller.Run;
    while Gate.Count = 0 loop delay 0.0; end loop;
    abort Abortable_Caller;
    if Is_Callable (Abortable_Caller'Identity) then raise Program_Error; end if;
    while not Is_Terminated (Abortable_Caller'Identity) loop delay 0.0; end loop;
    if Gate.Count /= 0 or Shared /= 10 or Bad then raise Program_Error; end if;
    Protected_Caller.Run;
    while Gate.Count = 0 loop delay 0.0; end loop;
    abort Protected_Caller;
    if Is_Callable (Protected_Caller'Identity) or Gate.Count /= 1 then raise Program_Error; end if;
    Gate.Open;
    while not Is_Terminated (Protected_Caller'Identity) loop delay 0.0; end loop;
    if Shared /= 13 or Bad then raise Program_Error; end if;
    Put_Line ("requeue cancellation OK");
end RequeueCancellation;
