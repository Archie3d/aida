with Ada.Text_IO;
with Ada.Exceptions;
procedure SharedImports is
    procedure Increment (value : in out Integer);
    pragma Import (C, Increment, "__test_shared_increment");
    procedure Fail (value : in out Integer);
    pragma Import (C, Fail, "__test_shared_fail");
    procedure Set (value : out Integer);
    pragma Import (C, Set, "__test_shared_out");
    value : Integer := 4;
    pragma Atomic (value);
    small : Integer range 0 .. 10 := 1;
    pragma Volatile (small);
begin
    Increment (value);
    if value /= 5 then
        raise Program_Error;
    end if;
    begin
        Fail (value);
        raise Program_Error;
    exception
        when e : Constraint_Error =>
            if value /= 5 or Ada.Exceptions.Exception_Message (e) /= "shared import failed" then
                raise Program_Error;
            end if;
    end;
    begin
        Set (small);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    if small /= 1 then
        raise Program_Error;
    end if;
    Set (value);
    if value /= 99 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("shared imports passed");
end SharedImports;
