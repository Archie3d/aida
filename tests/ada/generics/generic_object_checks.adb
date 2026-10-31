with Ada.Text_IO;
procedure Generic_Object_Checks is
    generic
        Value : Positive;
    package Checked is
        Saved : Integer := Value;
    end Checked;
    generic
        Value : in out Integer;
    procedure Clear;
    procedure Clear is
    begin
        Value := 0;
    end Clear;
    Positive_Value : Positive := 4;
    procedure Clear_Positive is new Clear (Positive_Value);
    Bad : Integer := -1;
    Caught : Integer := 0;
begin
    begin
        declare
            package Invalid is new Checked (Bad);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => Caught := Caught + 1;
    end;
    begin
        Clear_Positive;
        raise Program_Error;
    exception
        when Constraint_Error => Caught := Caught + 1;
    end;
    if Caught /= 2 or Positive_Value /= 4 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("generic object elaboration and reference constraint checks");
end Generic_Object_Checks;
