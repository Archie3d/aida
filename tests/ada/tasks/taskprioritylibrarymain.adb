with Ada.Text_IO;
with Ada.Dynamic_Priorities; use Ada.Dynamic_Priorities;
with TaskPriorityLibrary;
procedure TaskPriorityLibraryMain is
    -- The binder must apply this before library task creation/activation.
    pragma Priority (14);
    procedure No_Effect is
        pragma Priority (13);
    begin
        if Get_Priority /= 14 then raise Program_Error; end if;
    end No_Effect;
    protected Low_Ceiling is
        pragma Priority (13);
        procedure Check;
    end Low_Ceiling;
    protected body Low_Ceiling is
        procedure Check is begin raise Constraint_Error; end Check;
    end Low_Ceiling;
begin
    if Get_Priority /= 14 then raise Program_Error; end if;
    No_Effect;
    TaskPriorityLibrary.Worker.Check;
    begin Low_Ceiling.Check;
        raise Constraint_Error;
    exception when Program_Error => null; end;
    Set_Priority (13);
    declare
        task Child is entry Check; end Child;
        task body Child is
        begin
            if Get_Priority /= 13 then raise Program_Error; end if;
            accept Check;
        end Child;
    begin Child.Check; end;
    if Get_Priority /= 13 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("library and inherited priorities OK");
end TaskPriorityLibraryMain;
