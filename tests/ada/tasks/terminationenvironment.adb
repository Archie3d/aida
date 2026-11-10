with Ada.Text_IO;
with Ada.Task_Identification; use Ada.Task_Identification;
with Ada.Task_Termination; use Ada.Task_Termination;
with TerminationObserver;
procedure TerminationEnvironment is
begin
    Set_Specific_Handler (Current_Task, TerminationObserver.Log.Environment'Access);
    Ada.Text_IO.Put_Line ("main completed");
end TerminationEnvironment;
