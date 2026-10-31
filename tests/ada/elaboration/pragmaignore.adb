pragma Assertion_Policy (Ignore);
with Ada.Text_IO;
with Ada.Assertions;
procedure PragmaIgnore is
    Calls : Integer := 0;
    function Check return Boolean is
    begin
        Calls := Calls + 1;
        return False;
    end Check;
    generic
    package Model is
        function Valid return Boolean;
    end Model;
    package body Model is
        function Valid return Boolean is
        begin
            pragma Assert (Check);
            return True;
        end Valid;
    end Model;
    package Instance is new Model;
begin
    pragma Assert (Check);
    if not Instance.Valid or Calls /= 0 then raise Program_Error; end if;
    begin
        Ada.Assertions.Assert (False, "always enabled");
        raise Program_Error;
    exception
        when Ada.Assertions.Assertion_Error => null;
    end;
    Ada.Text_IO.Put_Line ("ignored assertions passed");
end PragmaIgnore;
