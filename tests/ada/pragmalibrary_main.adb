pragma Assertion_Policy (Ignore);
with Ada.Text_IO;
with Ada.Assertions;
with AssertionIgnoreLibrary;
with AssertionCheckLibrary;
procedure PragmaLibrary_Main is
    procedure Check is
        Calls : Integer := 0;
        function Condition return Boolean is
        begin
            Calls := Calls + 1;
            return False;
        end Condition;
        function Ignored is new AssertionIgnoreLibrary.Check (Condition);
        function Enabled is new AssertionCheckLibrary.Check (Condition);
    begin
        if not Ignored or Calls /= 0 then raise Program_Error; end if;
        begin
            if Enabled then raise Program_Error; end if;
        exception
            when Ada.Assertions.Assertion_Error => null;
        end;
        if Calls /= 1 then raise Program_Error; end if;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check;
    end Worker;
begin
    Check;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("library assertion policies passed");
end PragmaLibrary_Main;
