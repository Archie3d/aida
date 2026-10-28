with Ada.Text_IO;
with Ada.Assertions;
with Ada.Exceptions;
procedure PragmaPolicies is
    Calls : Integer := 0;
    type Pointer is access Integer;
    pragma Controlled (Pointer);
    Object : Pointer := new Integer'(42);
    function Message return String is
    begin
        Calls := Calls + 1;
        return "assertion message";
    end Message;
    function Bad_Message return String is
    begin
        raise Constraint_Error;
    end Bad_Message;
    function Bad_Check return Boolean is
    begin
        raise Constraint_Error;
    end Bad_Check;
    pragma Assert (Check => True, Message => Message);
    subtype Small is Integer range 1 .. 3;
    Value : Integer := 2;
    pragma Suppress (Range_Check);
    Before : Small := Value;
    pragma Unsuppress (Range_Check);
    After : Small := Value;
    procedure Falls_Through;
    pragma No_Return (Falls_Through);
    procedure Falls_Through is
    begin
        null;
    end Falls_Through;
begin
    pragma Assert (Object.all = 42);
    declare
        type Flag is new Boolean;
        Enabled : Flag := True;
    begin
        pragma Assert (Enabled);
    end;
    pragma Assert (Before = 2 and After = 2 and Calls = 0);
    begin
        pragma Assert (Check => False, Message => Message);
        raise Program_Error;
    exception
        when Error : Ada.Assertions.Assertion_Error =>
            if Ada.Exceptions.Exception_Message (Error) /= "assertion message" or Calls /= 1 then
                raise Program_Error;
            end if;
    end;
    begin
        pragma Assert (False, Bad_Message);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    begin
        pragma Assert (Bad_Check);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    begin
        Falls_Through;
        raise Constraint_Error;
    exception
        when Program_Error => null;
    end;
    Ada.Text_IO.Put_Line ("pragma policies passed");
end PragmaPolicies;
