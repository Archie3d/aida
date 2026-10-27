with Ada.Text_IO;
with GenericAttributeLibrary;
procedure GenericAttributeLibrary_Main is
    package Model is
        type Number is new Integer;
        subtype Narrow is Number range 1 .. 3;
    end Model;
    use type Model.Number;
    subtype Saved is Model.Narrow;
    package Numbers is new GenericAttributeLibrary (Model.Narrow);
    subtype Letters is Character range 'A' .. 'C';
    package Characters is new GenericAttributeLibrary (Letters);
    package Nested is new Numbers.Inner (Letters);
    package Second is new Characters.Inner (Model.Narrow);
    procedure Check is
        type T is (Wrong, Prefix);
        package Local is new GenericAttributeLibrary (Saved);
        package Model is
            type Number is (Different, Meaning);
        end Model;
    begin
        if Local.Step (2) /= 3 or Numbers.Step (2) /= 3 or Numbers.Following (3) /= 4 or Numbers.Roundtrip (2) /= 2
            or Characters.Step ('B') /= 'C' or Characters.Following ('C') /= 'D' or Characters.Roundtrip ('B') /= 'B'
            or Nested.Advance (3) /= 4 or not Nested.Check (2, 'B') or not Second.Check ('B', 2) then
            raise Program_Error;
        end if;
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
    Ada.Text_IO.Put_Line ("library attribute subprogram defaults passed");
end GenericAttributeLibrary_Main;
