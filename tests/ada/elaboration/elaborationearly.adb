with Ada.Text_IO; use Ada.Text_IO;
package body Elaborationearly is
    procedure Finalize (Object : in out Guard) is
    begin
        Put_Line ("library object finalized");
    end Finalize;
    Object : Guard;
    F : Callback := Later'Access;
    function Probe return Integer is
    begin
        return F.all;
    end Probe;
    X : Integer := Probe;
    function Later return Integer is
    begin
        Put_Line ("ERROR: premature body executed");
        return 42;
    end Later;
begin
    Put_Line ("ERROR: package body continued");
end Elaborationearly;
