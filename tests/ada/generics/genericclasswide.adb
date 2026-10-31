with Ada.Text_IO;
with GenericClasswideModel;
procedure GenericClasswide is
    package Model renames GenericClasswideModel;
    generic
        type T (<>) is new Model.Root with private;
    package Views is
        procedure Check (Object : in out T; Other : T);
    end Views;
    package body Views is
        function Renamed (Value : Integer := 3) return T renames Make;
        procedure Check (Object : in out T; Other : T) is
            Duplicate : T := Copy (Object);
            Errors : Integer := 0;
            Evaluations : Integer := 0;
            function Argument return Integer is
            begin
                Evaluations := Evaluations + 1;
                return 9;
            end Argument;
        begin
            if T'Definite or Read (Object => Duplicate) /= 12 then
                raise Program_Error;
            end if;
            Combine (Object, Make);
            Set (Object, 5);
            if Read (Object) /= 16 then
                raise Program_Error;
            end if;
            begin
                declare
                    Item : T := Make;
                begin
                    raise Constraint_Error;
                end;
            exception
                when Program_Error => Errors := Errors + 1;
            end;
            begin
                declare
                    Item : T := Renamed;
                begin
                    raise Constraint_Error;
                end;
            exception
                when Program_Error => Errors := Errors + 1;
            end;
            begin
                Combine (Object, Other);
            exception
                when Constraint_Error => Errors := Errors + 1;
            end;
            Object := Make (Value => 7);
            if Read (Object) /= 27 then
                raise Program_Error;
            end if;
            begin
                Object := Renamed (Value => 8);
                raise Constraint_Error;
            exception
                when Program_Error => Errors := Errors + 1;
            end;
            Object := Make;
            if Read (Object) /= 23 then
                raise Program_Error;
            end if;
            Object := Copy (Make);
            if Read (Object) /= 23 then
                raise Program_Error;
            end if;
            begin
                Set (Object, Read (Copy (Make)));
                raise Constraint_Error;
            exception
                when Program_Error => Errors := Errors + 1;
            end;
            begin
                declare
                    Item : T := Make (Value => Argument);
                begin
                    raise Constraint_Error;
                end;
            exception
                when Program_Error => Errors := Errors + 1;
            end;
            if Errors /= 6 or Evaluations /= 1 then
                raise Program_Error;
            end if;
        end Check;
    end Views;
    generic
        type T (<>) is new Model.Root with private;
    package Outer is
        generic
            type U (<>) is new T with private;
        function Nested_Read (Object : U) return Integer;
        package Inner is new Views (T);
    end Outer;
    package body Outer is
        function Nested_Read (Object : U) return Integer is
        begin
            return Read (Object);
        end Nested_Read;
    end Outer;
    package Outer_Instance is new Outer (Model.Root'Class);
    function Nested_Read is new Outer_Instance.Nested_Read (Model.Root'Class);
    package Instance is new Views (Model.Root'Class);
    procedure Check is
        Object : Model.Root'Class := Model.Child'(Model.Root'(Value => 2) with Extra => 10);
        Other : Model.Root := (Value => 0);
    begin
        if Nested_Read (Object) /= 12 then
            raise Program_Error;
        end if;
        Instance.Check (Object, Other);
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
    Ada.Text_IO.Put_Line ("generic class-wide actuals passed");
end GenericClasswide;
