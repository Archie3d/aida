with Ada.Text_IO; use Ada.Text_IO;
package body Controlled_Failed_Globals is
    function Set_Failure return Integer is
    begin
        Fail_Initialize := 4;
        return 0;
    end Set_Failure;
    Setup : Integer := Set_Failure;
    type Parts is array (1 .. 4) of Guard;
    Partial : Parts;
    Never_Created : Guard;
begin
    Put_Line ("unreachable package body");
end Controlled_Failed_Globals;
