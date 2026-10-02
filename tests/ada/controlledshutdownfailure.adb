with Controlled_Globals;
with Controlled_Shutdown_Model; use Controlled_Shutdown_Model;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledShutdownFailure is
begin
    Fail_Finalize := 7;
    Put_Line ("main done");
end ControlledShutdownFailure;
