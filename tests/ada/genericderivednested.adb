with Ada.Text_IO;
procedure GenericDerivedNested is
    package Model is
        type Root is range 0 .. 100;
        function Read (X : Root) return Integer;
        type Child is new Root;
        overriding function Read (X : Child) return Integer;
        type Grandchild is new Child;
    end Model;
    package body Model is
        function Read (X : Root) return Integer is
        begin
            return Integer (X) + 1;
        end Read;
        function Read (X : Child) return Integer is
        begin
            return Integer (X) + 50;
        end Read;
    end Model;
    generic
        type T is new Model.Root;
    package Outer is
        generic
            type U is new T;
        function Nested_Read (X : U) return Integer;
    end Outer;
    package body Outer is
        function Nested_Read (X : U) return Integer is
        begin
            return Read (X);
        end Nested_Read;
    end Outer;
    package Child_Outer is new Outer (Model.Child);
    function Read_Grandchild is new Child_Outer.Nested_Read (Model.Grandchild);
    package Root_Outer is new Outer (Model.Root);
    function Read_Root is new Root_Outer.Nested_Read (Model.Root);
    procedure Check is
    begin
        if Read_Grandchild (3) /= 4 or Read_Root (5) /= 6 then
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
    Ada.Text_IO.Put_Line ("nested generic derived types passed");
end GenericDerivedNested;
