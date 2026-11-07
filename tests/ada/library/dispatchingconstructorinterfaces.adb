with Ada.Tags.Generic_Dispatching_Constructor;
with Ada.Text_IO;
procedure DispatchingConstructorInterfaces is
    use type Ada.Tags.Tag;
    type Parameters is array (Positive range <>) of Integer;
    package Model is
        type First is interface;
        function Make (Params : not null access Parameters) return First is abstract;
        type Second is interface and First;
        type Root is tagged record
            Item : Integer := 0;
        end record;
        type Child is new Root and Second with record
            More : Integer := 0;
        end record;
        overriding function Make (Params : not null access Parameters) return Child;
    end Model;
    package body Model is
        function Make (Params : not null access Parameters) return Child is
        begin
            return (Model.Root'(Item => Params (Params'First)) with More => Params (Params'Last));
        end Make;
    end Model;
    function Create is new Ada.Tags.Generic_Dispatching_Constructor
        (Model.First, Parameters, Model.Make);
    P : aliased Parameters := (3 => 17, 4 => 29);
begin
    declare
        X : Model.First'Class := Create (Model.Child'Tag, P'Access);
    begin
        if X'Tag /= Model.Child'Tag or Model.Child (X).Item /= 17
            or Model.Child (X).More /= 29 then
            raise Program_Error;
        end if;
    end;
    Ada.Text_IO.Put_Line ("interface constructors passed");
end DispatchingConstructorInterfaces;
