with Ada.Text_IO;
with Ada.Task_Identification;
with Ada.Finalization;
procedure TaskIdentityLifecycle is
    use Ada.Task_Identification;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    protected State is
        procedure Set_Parent (Id : Task_Id);
        procedure Child_Started (Outer_Id, Inner_Id : Task_Id);
        entry Await_Child;
        entry Hold_Child;
        procedure Release_Child;
        procedure Finalizing;
        entry Await_Finalization;
        entry Hold_Finalizer;
        procedure Release_Finalizer;
        function Passed return Boolean;
    private
        Parent_Id : Task_Id;
        Child_Ready : Boolean := False;
        Child_Open : Boolean := False;
        Cleanup_Ready : Boolean := False;
        Cleanup_Open : Boolean := False;
        Valid : Boolean := False;
    end State;
    protected body State is
        procedure Set_Parent (Id : Task_Id) is
        begin
            Parent_Id := Id;
        end Set_Parent;
        procedure Child_Started (Outer_Id, Inner_Id : Task_Id) is
        begin
            Valid := Outer_Id = Parent_Id and Inner_Id = Current_Task and Inner_Id /= Parent_Id;
            Child_Ready := True;
        end Child_Started;
        entry Await_Child when Child_Ready is
        begin
            null;
        end Await_Child;
        entry Hold_Child when Child_Open is
        begin
            null;
        end Hold_Child;
        procedure Release_Child is
        begin
            Child_Open := True;
        end Release_Child;
        procedure Finalizing is
        begin
            Valid := Valid and not Is_Callable (Parent_Id) and not Is_Terminated (Parent_Id);
            Cleanup_Ready := True;
        end Finalizing;
        entry Await_Finalization when Cleanup_Ready is
        begin
            null;
        end Await_Finalization;
        entry Hold_Finalizer when Cleanup_Open is
        begin
            null;
        end Hold_Finalizer;
        procedure Release_Finalizer is
        begin
            Cleanup_Open := True;
        end Release_Finalizer;
        function Passed return Boolean is
        begin
            return Valid;
        end Passed;
    end State;
    package Guards is
        type Guard is new Ada.Finalization.Limited_Controlled with null record;
        overriding procedure Finalize (Object : in out Guard);
    end Guards;
    package body Guards is
        overriding procedure Finalize (Object : in out Guard) is
        begin
            State.Finalizing;
            State.Hold_Finalizer;
        end Finalize;
    end Guards;
    task type Parent;
    task body Parent is
        Guard : Guards.Guard;
        task type Child;
        task body Child is
            procedure Publish is
            begin
                State.Child_Started (Parent'Identity, Child'Identity);
            end Publish;
        begin
            Publish;
            State.Hold_Child;
        end Child;
        Dependent : Child;
    begin
        raise Constraint_Error;
    end Parent;
    Object : Parent;
    -- Initialized before the parent is activated, and read through State's lock.
    function Initialize return Boolean is
    begin
        State.Set_Parent (Object'Identity);
        return True;
    end Initialize;
    Initialized : Boolean := Initialize;
begin
    State.Await_Child;
    while Object'Callable loop
        delay 0.001;
    end loop;
    Check (not Object'Terminated);
    State.Release_Child;
    State.Await_Finalization;
    Check (not Object'Terminated and not Object'Callable);
    State.Release_Finalizer;
    while not Object'Terminated loop
        delay 0.001;
    end loop;
    Check (State.Passed);
    Ada.Text_IO.Put_Line ("task identity lifecycle passed");
end TaskIdentityLifecycle;
