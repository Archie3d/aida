with Ada.Text_IO; use Ada.Text_IO;
with Ada.Finalization;
with Ada.Task_Identification; use Ada.Task_Identification;
procedure AbortCleanup is
    Ready, Released, Initialized, Adjusted, Finalized, Action_Done : Boolean := False;
    Bad : Boolean := False;
    Fail_Finalize : Boolean := True;
    pragma Atomic (Ready);
    pragma Atomic (Released);
    pragma Atomic (Initialized);
    pragma Atomic (Adjusted);
    pragma Atomic (Finalized);
    pragma Atomic (Action_Done);
    pragma Atomic (Bad);
    pragma Atomic (Fail_Finalize);
    type Item is new Ada.Finalization.Controlled with null record;
    overriding procedure Initialize (Object : in out Item);
    overriding procedure Adjust (Object : in out Item);
    overriding procedure Finalize (Object : in out Item);
    overriding procedure Initialize (Object : in out Item) is
    begin
        Ready := True;
        while not Released loop delay 0.0; end loop;
        Initialized := True;
    end Initialize;
    overriding procedure Adjust (Object : in out Item) is
    begin
        Ready := True;
        while not Released loop delay 0.0; end loop;
        Adjusted := True;
    end Adjust;
    overriding procedure Finalize (Object : in out Item) is
    begin
        Finalized := True;
        -- Exceptions during abort-driven finalization are ignored.
        if Fail_Finalize then raise Constraint_Error; end if;
    end Finalize;
    task Initialize_Worker;
    task body Initialize_Worker is
    begin
        declare Object : Item;
        begin delay 60.0; Bad := True; end;
    exception when others => Bad := True;
    end Initialize_Worker;
begin
    while not Ready loop delay 0.0; end loop;
    abort Initialize_Worker;
    if Is_Callable (Initialize_Worker'Identity) then raise Program_Error; end if;
    Released := True;
    while not Is_Terminated (Initialize_Worker'Identity) loop delay 0.0; end loop;
    if not Initialized or not Finalized or Bad then raise Program_Error; end if;
    -- Use a fresh scope after the first worker, so the synchronization flags
    -- identify an assignment's Adjust rather than its default initialization.
    Ready := False;
    Finalized := False;
    Released := True;
    Fail_Finalize := False;
    declare
        task Assignment_Worker is entry Run; end Assignment_Worker;
        task body Assignment_Worker is
            Source, Target : Item;
        begin
            accept Run;
            Target := Source;
            Bad := True;
        exception when others => Bad := True;
        end Assignment_Worker;
    begin
        Ready := False;
        Released := False;
        Assignment_Worker.Run;
        while not Ready loop delay 0.0; end loop;
        Fail_Finalize := True;
        abort Assignment_Worker;
        Released := True;
        while not Is_Terminated (Assignment_Worker'Identity) loop delay 0.0; end loop;
        if not Adjusted or not Finalized or Bad then raise Program_Error; end if;
    end;
    Ready := False;
    Released := False;
    declare
        protected Gate is
            entry Hold;
            procedure Reject_Abort;
        end Gate;
        protected body Gate is
            procedure Reject_Abort is
            begin
                abort Initialize_Worker;
            end Reject_Abort;
            entry Hold when True is
            begin
                Ready := True;
                while not Released loop null; end loop;
                Action_Done := True;
            end Hold;
        end Gate;
        task Caller;
        task body Caller is
        begin Gate.Hold; Bad := True;
        exception when others => Bad := True;
        end Caller;
    begin
        while not Ready loop delay 0.0; end loop;
        abort Caller;
        Released := True;
        while not Is_Terminated (Caller'Identity) loop delay 0.0; end loop;
        if not Action_Done or Bad then raise Program_Error; end if;
        begin
            Gate.Reject_Abort;
            raise Constraint_Error;
        exception when Program_Error => null;
        end;
    end;
    Put_Line ("abort cleanup OK");
end AbortCleanup;
