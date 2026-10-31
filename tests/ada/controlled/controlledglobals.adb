with Controlled_Globals; use Controlled_Globals;
with Controlled_Shutdown_Model; use Controlled_Shutdown_Model;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledGlobals is
    Local : Guard;
begin
    if Live /= 10 then
        raise Program_Error;
    end if;
    Alias := Copy;
    if Live /= 10 then
        raise Program_Error;
    end if;
    Put_Line ("main done");
end ControlledGlobals;
