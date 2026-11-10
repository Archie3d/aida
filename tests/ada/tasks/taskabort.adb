with Ada.Text_IO; use Ada.Text_IO;
with Ada.Task_Identification; use Ada.Task_Identification;
procedure TaskAbort is
    Bad, Child_Ready : Boolean := False;
    pragma Atomic (Bad);
    pragma Atomic (Child_Ready);
    task Idle is entry Never; end Idle;
    task body Idle is
    begin
        accept Never;
        Bad := True;
    exception when others => Bad := True;
    end Idle;
    task Parent is entry Ready; end Parent;
    task body Parent is
        task Child;
        task body Child is
        begin
            Child_Ready := True;
            loop delay 60.0; Bad := True; end loop;
        exception when others => Bad := True;
        end Child;
    begin
        accept Ready;
        delay 60.0;
        Bad := True;
    exception when others => Bad := True;
    end Parent;
    task Self;
    task body Self is
    begin
        abort Self;
        Bad := True;
    exception when others => Bad := True;
    end Self;
    task Finished;
    task body Finished is begin null; end Finished;
    task type Worker;
    task body Worker is
    begin
        loop delay 60.0; Bad := True; end loop;
    exception when others => Bad := True;
    end Worker;
    type Workers is array (Positive range <>) of Worker;
    Group : Workers (1 .. 2);
    type Reference is access Worker;
    Pointer : Reference := new Worker;
begin
    Parent.Ready;
    while not Child_Ready loop delay 0.0; end loop;
    abort Idle, Parent, Group (1), Pointer.all;
    Abort_Task (Group (2)'Identity);
    if Is_Callable (Idle'Identity) or Is_Callable (Parent'Identity)
        or Is_Callable (Group (1)'Identity) or Is_Callable (Pointer.all'Identity) then
        raise Program_Error;
    end if;
    while not Is_Terminated (Idle'Identity) or not Is_Terminated (Parent'Identity)
        or not Is_Terminated (Self'Identity) or not Is_Terminated (Finished'Identity)
        or not Is_Terminated (Group (1)'Identity) or not Is_Terminated (Group (2)'Identity)
        or not Is_Terminated (Pointer.all'Identity) loop delay 0.0; end loop;
    -- Repeated aborts of completed/terminated tasks do not affect their peers.
    abort Finished, Parent, Self;
    Abort_Task (Group (2)'Identity);
    begin Abort_Task (Null_Task_Id); raise Constraint_Error;
    exception when Program_Error => null;
    end;
    if Bad then raise Program_Error; end if;
    -- An aborted accepting task releases its selected caller exceptionally,
    -- without copying back the unfinished scalar formal.
    declare
        Ready : Boolean := False;
        pragma Atomic (Ready);
        Value : Integer := 7;
        task Server is entry Work (N : in out Integer); end Server;
        task body Server is
        begin
            accept Work (N : in out Integer) do
                N := 99;
                Ready := True;
                delay 60.0;
                Bad := True;
            end Work;
        exception when others => Bad := True;
        end Server;
        task Aborter;
        task body Aborter is
        begin
            while not Ready loop delay 0.0; end loop;
            abort Server;
        end Aborter;
    begin
        begin Server.Work (Value); raise Program_Error;
        exception when Tasking_Error => null;
        end;
        if Value /= 7 or Bad then raise Program_Error; end if;
    end;
    Put_Line ("task abort OK");
end TaskAbort;
