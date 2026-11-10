with Ada.Text_IO;
with Ada.Dynamic_Priorities; use Ada.Dynamic_Priorities;
with Ada.Task_Identification; use Ada.Task_Identification;
with System;
procedure TaskPriority is
    pragma Priority (15);
    task type Worker (Bytes : Integer; P : Integer) is
        pragma Priority (P);
        pragma Storage_Size (Bytes);
        entry Check;
    end Worker;
    task body Worker is
    begin
        if Get_Priority /= P or else Worker'Storage_Size < Bytes then raise Program_Error; end if;
        accept Check;
    end Worker;
    A : aliased Worker (131073, 15);
    type Ref is access all Worker;
    R : Ref := A'Access;
    protected Box is
        pragma Priority (15);
        procedure Check;
    end Box;
    protected body Box is
        procedure Check is
        begin
            if Get_Priority /= 15 then raise Program_Error; end if;
        end Check;
    end Box;
begin
    if R'Storage_Size < 131073 or else A'Storage_Size < 131073 then raise Program_Error; end if;
    if Get_Priority /= System.Default_Priority then raise Program_Error; end if;
    Set_Priority (15);
    Box.Check;
    A.Check;
    while not A'Terminated loop delay 0.0; end loop;
    Set_Priority (15, A'Identity);
    begin
        if Get_Priority (A'Identity) = 15 then null; end if;
        raise Constraint_Error;
    exception when Tasking_Error => null; end;
    begin
        declare
            Bad : Worker (131073, 31);
        begin raise Program_Error; end;
        raise Program_Error;
    exception when Constraint_Error => null; end;
    begin
        declare
            Bad : Worker (-1, 15);
        begin raise Program_Error; end;
        raise Program_Error;
    exception when Storage_Error => null; end;
    begin
        Set_Priority (15, Null_Task_Id);
        raise Constraint_Error;
    exception when Program_Error => null; end;
    Ada.Text_IO.Put_Line ("task priorities and storage OK");
end TaskPriority;
