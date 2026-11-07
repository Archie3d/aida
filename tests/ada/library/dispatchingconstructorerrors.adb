with Ada.Tags.Generic_Dispatching_Constructor;
procedure DispatchingConstructorErrors is
    type Parameters is null record;
    package Model is
        type Root is abstract tagged null record;
        function Make (Params : not null access Parameters) return Root is abstract;
    end Model;
    function Create is new Ada.Tags.Generic_Dispatching_Constructor
        (Model.Root, Parameters, Model.Make);
    function Plain (Params : not null access Parameters) return Model.Root is abstract;
    function Invalid is new Ada.Tags.Generic_Dispatching_Constructor
        (Model.Root, Parameters, Plain);
begin
    null;
end DispatchingConstructorErrors;
