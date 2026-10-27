with Ada.Text_IO;
procedure GenericAbstractOperators is
    package Model is
        type Root is tagged record
            Value : Integer;
        end record;
        function Less (Left, Right : Root) return Boolean;
        type Child is new Root with null record;
        overriding function Less (Left, Right : Child) return Boolean;
    end Model;
    package body Model is
        function Less (Left, Right : Root) return Boolean is
        begin
            return Left.Value < Right.Value;
        end Less;
        function Less (Left, Right : Child) return Boolean is
        begin
            return Left.Value > Right.Value;
        end Less;
    end Model;
    generic
        type T is tagged private;
        with function "=" (Left, Right : T) return Boolean is abstract;
    function Compare (Left, Right : T'Class) return Boolean;
    function Compare (Left, Right : T'Class) return Boolean is
    begin
        return Left = Right and not (Left /= Right);
    end Compare;
    function Check_Root is new Compare (Model.Root, Model.Less);
    function Check_Child is new Compare (Model.Child, Model.Less);
    procedure Check is
        A : Model.Child := (Value => 3);
        B : Model.Child := (Value => 2);
    begin
        if not Check_Root (A, B) or not Check_Child (A, B) then
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
    Ada.Text_IO.Put_Line ("abstract formal operators passed");
end GenericAbstractOperators;
