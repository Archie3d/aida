with Ada.Text_IO;
with Ada.Task_Termination; use Ada.Task_Termination;
with TerminationObserver; use TerminationObserver;
procedure TerminationCollections is
    task type Shared_Child;
    task body Shared_Child is begin null; end Shared_Child;
    type Shared_Ref is access Shared_Child;
    P : Shared_Ref;
    task Foreign_Allocator;
    task body Foreign_Allocator is
    begin
        Set_Dependents_Fallback_Handler (Log.Note'Access);
        -- This collection belongs to the environment task, not the allocator.
        P := new Shared_Child;
    end Foreign_Allocator;
    task Parent;
    task body Parent is
    begin
        Set_Dependents_Fallback_Handler (Log.Note'Access);
        declare
            task type Child;
            task body Child is begin null; end Child;
            type Ref is access Child;
            P : Ref := new Child;
        begin null; end;
    end Parent;
begin
    while not Parent'Terminated loop delay 0.0; end loop;
    while not Foreign_Allocator'Terminated loop delay 0.0; end loop;
    while not P'Terminated loop delay 0.0; end loop;
    if Log.Count (Normal) /= 1 or else Log.Failed then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("termination collection masters OK");
end TerminationCollections;
