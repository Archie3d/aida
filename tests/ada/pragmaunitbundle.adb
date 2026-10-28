pragma Assertion_Policy (Ignore);
package PragmaUnitModel is
    function Ignored return Boolean;
end PragmaUnitModel;
package body PragmaUnitModel is
    function Ignored return Boolean is
    begin
        pragma Assert (False);
        return True;
    end Ignored;
end PragmaUnitModel;
pragma Assertion_Policy (Check);
with Ada.Text_IO;
with Ada.Assertions;
with PragmaUnitModel;
procedure PragmaUnitBundle is
    procedure Child is separate;
begin
    if not PragmaUnitModel.Ignored then raise Program_Error; end if;
    Child;
    begin
        pragma Assert (False);
        raise Program_Error;
    exception
        when Ada.Assertions.Assertion_Error => null;
    end;
    Ada.Text_IO.Put_Line ("unit and subunit policies passed");
end PragmaUnitBundle;
