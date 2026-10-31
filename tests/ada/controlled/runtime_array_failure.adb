with Ada.Text_IO; use Ada.Text_IO;
package body Runtime_Array_Failure is
    function Bound return Integer is
    begin
        Fail_Initialize := 4;
        return 4;
    end Bound;
    type Parts is array (1 .. Bound) of Guard;
    Partial : Parts;
    Never_Created : Guard;
begin
    Put_Line ("unreachable package body");
end Runtime_Array_Failure;
