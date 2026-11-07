with Ada.Tags.Generic_Dispatching_Constructor;
with Ada.Text_IO;
procedure DispatchingConstructorDiscriminants is
    type Parameters is record
        Length : Positive;
    end record;
    package Model is
        type Root (Length : Positive) is abstract tagged record
            Data : String (1 .. Length);
        end record;
        function Make (Params : not null access Parameters) return Root is abstract;
        type Child is new Root with record
            Value : Integer := 29;
        end record;
        overriding function Make (Params : not null access Parameters) return Child;
    end Model;
    package body Model is
        function Make (Params : not null access Parameters) return Child is
        begin
            return Result : Child (Params.Length) do
                Result.Data := (others => 'x');
            end return;
        end Make;
    end Model;
    function Create is new Ada.Tags.Generic_Dispatching_Constructor
        (Model.Root, Parameters, Model.Make);
    P : aliased Parameters := (Length => 37);
begin
    declare
        X : Model.Root'Class := Create (Model.Child'Tag, P'Access);
    begin
        if X.Length /= 37 or X.Data'Length /= 37 or X.Data (37) /= 'x'
            or Model.Child (X).Value /= 29 then
            raise Program_Error;
        end if;
    end;
    Ada.Text_IO.Put_Line ("discriminated constructors passed");
end DispatchingConstructorDiscriminants;
