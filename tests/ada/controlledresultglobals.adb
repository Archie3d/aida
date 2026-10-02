with Controlled_Result_Globals;
with Controlled_Result_Model; use Controlled_Result_Model;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledResultGlobals is
begin
    Check (Objects = 1 and Resources = 1);
    Check (Controlled_Result_Globals.Object.Data.Value = 42);
    Put_Line ("global result alive");
end ControlledResultGlobals;
