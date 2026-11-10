with Ada.Text_IO;
with Ada.Finalization;
with Ada.Task_Attributes;
with Ada.Task_Identification; use Ada.Task_Identification;
with Ada.Synchronous_Task_Control; use Ada.Synchronous_Task_Control;
procedure TaskAttributesCleanup is
    Task_Finalized, Instance_Finalized : Boolean := False;
    pragma Atomic (Task_Finalized);
    pragma Atomic (Instance_Finalized);
    type Item is new Ada.Finalization.Controlled with record
        Id : Integer := 0;
    end record;
    overriding procedure Finalize (Object : in out Item) is
    begin
        if Object.Id = 42 then Task_Finalized := True;
        elsif Object.Id = 77 then Instance_Finalized := True;
        end if;
    end Finalize;
    Initial : Item;
    package Values is new Ada.Task_Attributes (Item, Initial);
    Gate : Suspension_Object;
    task Worker is entry Ready; end Worker;
    task body Worker is
        P : Values.Attribute_Handle := Values.Reference;
    begin
        P.Id := 42;
        accept Ready;
        Suspend_Until_True (Gate);
    end Worker;
    P : Values.Attribute_Handle;
    N : Item;
begin
    Worker.Ready;
    -- A late instance also covers an already running task.
    declare
        package Late is new Ada.Task_Attributes (Integer, 19);
        Q : Late.Attribute_Handle := Late.Reference (Worker'Identity);
    begin
        if Q.all /= 19 then raise Program_Error; end if;
        Late.Set_Value (21, Worker'Identity);
        if Late.Value (Worker'Identity) /= 21 then raise Program_Error; end if;
    end;
    P := Values.Reference;
    P.Id := 77;
    Values.Reinitialize;
    if not Instance_Finalized then raise Program_Error; end if;
    N := Values.Value;
    if N.Id /= 0 then raise Program_Error; end if;
    Set_True (Gate);
    while not Worker'Terminated loop delay 0.0; end loop;
    -- Native termination publishes Terminated before running finalizers;
    -- use a synchronized atomic observation for their completion.
    while not Task_Finalized loop delay 0.0; end loop;
    begin
        N := Values.Value (Worker'Identity);
        raise Constraint_Error;
    exception when Tasking_Error => null; end;
    begin
        N := Values.Value (Null_Task_Id);
        raise Constraint_Error;
    exception when Program_Error => null; end;
    Instance_Finalized := False;
    declare
        package Local is new Ada.Task_Attributes (Item, Initial);
        Q : Local.Attribute_Handle := Local.Reference;
    begin Q.Id := 77; end;
    if not Instance_Finalized then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("task attribute cleanup OK");
end TaskAttributesCleanup;
