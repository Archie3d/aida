procedure AbstractDeclarationErrors is
    package Model is
        type Concrete is tagged null record;
        procedure Bad (Self : Concrete) is abstract;
        type Root is abstract tagged null record;
        procedure Act (Self : Root) is abstract;
        procedure Complete (Self : Root);
        function Make return Root;
        function Access_Result return access Root is abstract;
        type Missing is new Root with null record;
        type Still_Abstract is abstract new Root with null record;
        overriding procedure Act (Self : Still_Abstract) is abstract;
        type Missing_Again is new Still_Abstract with null record;
        type Invalid_Component is record
            Item : Root;
        end record;
        type Invalid_Array is array (1 .. 2) of Root;
        type Hidden_Abstract is abstract tagged null record;
        type Hidden_Concrete is tagged private;
    private
        type Hidden_Concrete is abstract tagged null record;
        procedure Hidden (Self : Hidden_Abstract) is abstract;
    end Model;
    package body Model is
        procedure Act (Self : Root) is null;
        procedure Complete (Self : Root) renames Act;
        function Make return Root is
        begin
            raise Program_Error;
        end Make;
    end Model;
begin
    null;
end AbstractDeclarationErrors;
