with Ada.Command_Line;
procedure CommandLineFailure is
begin
    Ada.Command_Line.Set_Exit_Status (Ada.Command_Line.Success);
    raise Program_Error;
end CommandLineFailure;
