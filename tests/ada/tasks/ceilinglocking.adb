with CeilingModel;
with Ada.Text_IO;
with Ada.Dynamic_Priorities;
with Ada.Synchronous_Task_Control;
procedure CeilingLocking is
    function Active return Integer;
    pragma Import (C, Active, "__ada_test_active_priority");
    signal : Ada.Synchronous_Task_Control.Suspension_Object;
    protected Normal is
        function Level return Integer;
        procedure Fail;
    end Normal;
    protected body Normal is
        function Level return Integer is
        begin
            return Active;
        end Level;
        procedure Fail is
        begin
            if Active /= 30 then raise Program_Error; end if;
            raise Constraint_Error;
        end Fail;
    end Normal;
    protected Low is
        pragma Priority (20);
        function Level return Integer;
    end Low;
    protected body Low is
        function Level return Integer is
        begin
            return Active;
        end Level;
    end Low;
    protected High is
        pragma Interrupt_Priority;
        procedure Check;
        procedure Change;
    end High;
    protected body High is
        procedure Check is
            caught : Boolean := False;
            ignored : Integer;
        begin
            if Active /= 31 then raise Program_Error; end if;
            begin
                ignored := Normal.Level;
            exception
                when Program_Error => caught := True;
            end;
            if not caught or Active /= 31 then raise Program_Error; end if;
            Ada.Synchronous_Task_Control.Set_True (signal);
            if not Ada.Synchronous_Task_Control.Current_State (signal) then
                raise Program_Error;
            end if;
            Ada.Synchronous_Task_Control.Set_False (signal);
        end Check;
        procedure Change is
        begin
            Ada.Dynamic_Priorities.Set_Priority (20);
            if Active /= 31 then raise Program_Error; end if;
        end Change;
    end High;
    task type Worker is
        pragma Interrupt_Priority (Expression => 31);
        entry Done;
    end Worker;
    task body Worker is
        caught : Boolean := False;
        ignored : Integer;
    begin
        if Active /= 31 then raise Program_Error; end if;
        for i in 1 .. 5 loop
            High.Check;
            begin
                ignored := Normal.Level;
            exception
                when Program_Error => caught := True;
            end;
            if not caught or Active /= 31 then raise Program_Error; end if;
        end loop;
        accept Done do
            if Active /= 31 then raise Program_Error; end if;
        end Done;
    end Worker;
    task LowWorker is
        pragma Interrupt_Priority (15);
        entry Done;
    end LowWorker;
    task body LowWorker is
    begin
        if Active /= 15 or Normal.Level /= 30 then raise Program_Error; end if;
        accept Done;
    end LowWorker;
begin
    if CeilingModel.Object.Level /= 30 then raise Program_Error; end if;
    if Active /= 15 or Normal.Level /= 30 or Low.Level /= 20 or Active /= 15 then
        raise Program_Error;
    end if;
    begin
        Normal.Fail;
    exception
        when Constraint_Error => null;
    end;
    if Active /= 15 then raise Program_Error; end if;
    High.Check;
    if Active /= 15 then raise Program_Error; end if;
    High.Change;
    if Active /= 20 or Low.Level /= 20 or Active /= 20 then
        raise Program_Error;
    end if;
    Ada.Dynamic_Priorities.Set_Priority (15);
    declare
        workers : array (1 .. 4) of Worker;
    begin
        for i in workers'Range loop
            workers (i).Done;
        end loop;
    end;
    LowWorker.Done;
    if Active /= 15 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("ceiling locking OK");
end CeilingLocking;
