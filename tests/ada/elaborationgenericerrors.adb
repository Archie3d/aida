with Ada.Text_IO; use Ada.Text_IO;
procedure Elaborationgenericerrors is
    generic
    function G return Integer;
    function Probe return Integer is
        function Instance is new G;
    begin
        return 0;
    exception
        when Program_Error => return 0;
    end Probe;
    X : Integer := Probe;
    function G return Integer is
    begin
        return 42;
    end G;
begin
    Put_Line (Integer'Image (X));
end Elaborationgenericerrors;
