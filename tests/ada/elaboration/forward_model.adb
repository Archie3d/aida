with Ada.Text_IO; use Ada.Text_IO;
package body Forward_Model is
    function Invoke (Value : Integer) return Integer is
        function Instance is new Template (Value);
    begin
        return Instance;
    end Invoke;
    function Recover return Integer is
    begin
        return Invoke (1);
    exception
        when Program_Error =>
            Put_Line ("library template checked");
            return 0;
    end Recover;
    X : Integer := Recover;
    Offset : Integer := 40;
    function Template return Integer is
    begin
        return Value + Offset;
    end Template;
begin
    Early := X;
end Forward_Model;
