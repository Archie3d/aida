with Ada.Text_IO;
with GenericAbstractModel;
procedure GenericAbstract is
    package Model renames GenericAbstractModel;
    generic
        type T is abstract tagged private;
        with function Score (Object : T; Extra : Integer := 5) return Integer is abstract;
        with procedure Update (Object : in out T; Amount : Integer := 4) is abstract;
        with function Construct (Value : Integer := 7) return T is abstract;
        with function Pointer (Object : access T) return Integer is abstract;
    package Views is
        procedure Check (Object : in out T'Class);
    end Views;
    package body Views is
        procedure Check (Object : in out T'Class) is
        begin
            if Score (Object => Object) /= 6 then
                raise Program_Error;
            end if;
            Update (Object => Object);
            if Pointer (Object'Access) /= 15 then
                raise Program_Error;
            end if;
            Object := Construct;
            if Score (Object, Extra => 1) /= 8 then
                raise Program_Error;
            end if;
            begin
                declare
                    Invalid : Integer := Score (Object, Extra => 0);
                begin
                    raise Program_Error;
                end;
            exception
                when Constraint_Error => null;
            end;
        end Check;
    end Views;
    function Alias_Read (Object : Model.Child; Extra : Integer := 5) return Integer renames Model.Read;
    package Alias_View is new Views (Model.Child, Alias_Read, Model.Change, Model.Make, Model.Pointer_Read);
    package Instance is new Views (Model.Root, Model.Read, Model.Change, Model.Make, Model.Pointer_Read);
    package Concrete is new Views (Model.Child, Model.Read, Model.Change, Model.Make, Model.Pointer_Read);
    procedure Check is
        Object : Model.Root'Class := Model.Child'(Value => 1);
        Child : Model.Child := (Value => 1);
    begin
        Instance.Check (Object);
        Concrete.Check (Child);
        Child.Value := 1;
        Alias_View.Check (Child);
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
    Ada.Text_IO.Put_Line ("abstract formal subprograms passed");
end GenericAbstract;
