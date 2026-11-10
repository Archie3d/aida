with Ada.Text_IO;
with Ada.Synchronous_Task_Control; use Ada.Synchronous_Task_Control;
procedure SuspensionControl is
    Gate : Suspension_Object;
    protected Signaller is
        procedure Signal;
        procedure Invalid_Wait;
    end Signaller;
    protected body Signaller is
        procedure Signal is begin Set_False (Gate); Set_True (Gate); end Signal;
        procedure Invalid_Wait is begin Suspend_Until_True (Gate); end Invalid_Wait;
    end Signaller;
    task Waiter is entry Ready; end Waiter;
    task body Waiter is
    begin
        accept Ready;
        Suspend_Until_True (Gate);
        raise Program_Error;
    end Waiter;
begin
    Waiter.Ready;
    abort Waiter;
    while not Waiter'Terminated loop delay 0.0; end loop;
    if Current_State (Gate) then raise Program_Error; end if;
    begin
        Signaller.Invalid_Wait;
        raise Constraint_Error;
    exception when Program_Error => null; end;
    Signaller.Signal;
    if not Current_State (Gate) then raise Program_Error; end if;
    Suspend_Until_True (Gate);
    if Current_State (Gate) then raise Program_Error; end if;
    select
        delay 0.01;
    then abort
        Suspend_Until_True (Gate);
        raise Program_Error;
    end select;
    Set_True (Gate);
    Suspend_Until_True (Gate);
    Set_True (Gate);
    Set_False (Gate);
    if Current_State (Gate) then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("suspension control OK");
end SuspensionControl;
