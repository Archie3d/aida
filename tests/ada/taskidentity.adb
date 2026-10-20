with Ada.Text_IO;
with Ada.Task_Identification;
procedure TaskIdentity is
    use Ada.Task_Identification;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    Environment : constant Task_Id := Current_Task;
    Default_Id : Task_Id;
    type Id_Array is array (1 .. 2) of Task_Id;
    Defaults : Id_Array;
    protected Gate is
        entry Wait;
        procedure Open;
        procedure Success;
        function Count return Integer;
    private
        Ready : Boolean := False;
        Passed : Integer := 0;
    end Gate;
    protected body Gate is
        entry Wait when Ready is
            function Caller return Task_Id is
            begin
                return Wait'Caller;
            end Caller;
        begin
            Check (Caller = Current_Task);
        end Wait;
        procedure Open is
        begin
            Ready := True;
        end Open;
        procedure Success is
        begin
            Passed := Passed + 1;
        end Success;
        function Count return Integer is
        begin
            return Passed;
        end Count;
    end Gate;
    task type Worker;
    task body Worker is
        function Own_Id return Task_Id is
        begin
            return Worker'Identity;
        end Own_Id;
    begin
        Check (Own_Id = Current_Task and Worker'Callable and not Worker'Terminated);
        Check (Own_Id /= Environment);
        Gate.Wait;
        Gate.Success;
    end Worker;
    First, Second : Worker;
    Before : constant Task_Id := First'Identity;
    Was_Callable : constant Boolean := First'Callable;
    Was_Terminated : constant Boolean := First'Terminated;
    Alias : Worker renames First;
    type Pointer is access Worker;
    Allocated : Pointer := new Worker;
    Missing : Pointer;
    Calls : Integer := 0;
    function Get return Pointer is
    begin
        Calls := Calls + 1;
        return Allocated;
    end Get;
    procedure Inspect (Object : Worker) is
    begin
        Check (Object'Identity = Before);
    end Inspect;
begin
    Check (Default_Id = Null_Task_Id and Defaults (1) = Null_Task_Id);
    Check (Defaults (2) = Null_Task_Id and Image (Null_Task_Id) = "");
    Check (Was_Callable and not Was_Terminated);
    Check (Environment /= Null_Task_Id and Is_Callable (Environment) and not Is_Terminated (Environment));
    Check (Before = First'Identity and Alias'Identity = Before);
    Check (Ada.Task_Identification."=" (Before, First'Identity));
    Check (First'Identity /= Second'Identity);
    Check (Image (First'Identity) /= Image (Second'Identity));
    Check (Image (Before) = Image (First'Identity));
    Check (Image (Before)'Length > 0);
    Check (Get'Identity = Allocated.all'Identity and Calls = 1);
    Inspect (First);
    begin
        Check (Missing'Callable);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    begin
        Check (Is_Callable (Null_Task_Id));
        raise Constraint_Error;
    exception
        when Program_Error => null;
    end;
    begin
        Check (Is_Terminated (Null_Task_Id));
        raise Constraint_Error;
    exception
        when Program_Error => null;
    end;
    Gate.Open;
    Gate.Wait;
    while not First'Terminated or not Second'Terminated or not Allocated'Terminated loop
        delay 0.001;
    end loop;
    Check (not First'Callable and not Is_Callable (Before) and Is_Terminated (Before));
    Check (Gate.Count = 3);
    Ada.Text_IO.Put_Line ("task identity passed");
end TaskIdentity;
