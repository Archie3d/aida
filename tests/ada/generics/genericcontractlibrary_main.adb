with Ada.Text_IO;
with GenericClasswideModel;
with GenericContractLibrary;
with GenericContractArithmetic;
procedure GenericContractLibrary_Main is
    package Model renames GenericClasswideModel;
    package Wide is new GenericContractLibrary (Model.Root'Class);
    package Specific is new GenericContractLibrary (Model.Child);
    function Wide_Nested is new Wide.Nested_Score;
    function Specific_Nested is new Specific.Nested_Score;
    package First is new GenericContractArithmetic (Integer);
    package Second is new GenericContractArithmetic (Integer);
    function A is new First.Sum (3);
    function B is new Second.Sum (4);
    procedure Check is
        type T is (Wrong, Scope);
        Value : Model.Child := (Value => 3, Extra => 10);
    begin
        if Wide.Score (Value) /= 13 or Specific.Score (Value) /= 13
            or Wide_Nested (Value) /= 13 or Specific_Nested (Value) /= 13
            or A /= 6 or B /= 8 then
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
    Ada.Text_IO.Put_Line ("library generic contracts passed");
end GenericContractLibrary_Main;
