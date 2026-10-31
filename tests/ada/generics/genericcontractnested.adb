with Ada.Text_IO;
with GenericClasswideModel;
procedure GenericContractNested is
    package Model renames GenericClasswideModel;
    generic
        type U (<>) is tagged private;
        with function Measure (Object : U) return Integer is abstract;
        with function Clone (Object : U) return U is abstract;
    function Inner (Object : U'Class) return Integer;
    function Inner (Object : U'Class) return Integer is
    begin
        return Measure (Clone (Object));
    end Inner;
    generic
        type T (<>) is new Model.Root with private;
    package Outer is
        function Score (Object : T'Class) return Integer;
    end Outer;
    package body Outer is
        function Apply is new Inner (T, Read, Copy);
        function Score (Object : T'Class) return Integer is
        begin
            return Apply (Object);
        end Score;
    end Outer;
    package Wide is new Outer (Model.Root'Class);
    package Specific is new Outer (Model.Child);
    procedure Check is
        Value : Model.Child := (Value => 3, Extra => 10);
    begin
        if Wide.Score (Value) /= 13 or Specific.Score (Value) /= 13 then
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
    Ada.Text_IO.Put_Line ("nested generic contracts passed");
end GenericContractNested;
