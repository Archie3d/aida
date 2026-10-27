with Ada.Text_IO;
procedure GenericAbstractInterfaces is
    package Model is
        type First is interface;
        function Read (Object : First) return Integer is abstract;
        type Second is interface;
        procedure Change (Object : in out Second) is abstract;
        type Child is new Second and First with record
            Value : Integer := 2;
        end record;
        overriding function Read (Object : Child) return Integer;
        overriding procedure Change (Object : in out Child);
    end Model;
    package body Model is
        function Read (Object : Child) return Integer is
        begin
            return Object.Value;
        end Read;
        procedure Change (Object : in out Child) is
        begin
            Object.Value := Object.Value + 1;
        end Change;
    end Model;
    generic
        type T is interface;
        with function Score (Object : T) return Integer is abstract;
    function Read_View (Object : T'Class) return Integer;
    function Read_View (Object : T'Class) return Integer is
    begin
        return Score (Object => Object);
    end Read_View;
    function Score is new Read_View (Model.First, Model.Read);
    generic
        with procedure Update (Object : in out Model.Second) is abstract Model.Change;
    procedure Change_View (Object : in out Model.Second'Class);
    procedure Change_View (Object : in out Model.Second'Class) is
    begin
        Update (Object => Object);
    end Change_View;
    procedure Update is new Change_View;
    procedure Check is
        Object : Model.Child;
    begin
        Update (Object);
        if Score (Object) /= 3 then
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
    Ada.Text_IO.Put_Line ("interface abstract formal subprograms passed");
end GenericAbstractInterfaces;
