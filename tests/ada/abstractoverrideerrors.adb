procedure AbstractOverrideErrors is
    package Model is
        type Root is abstract tagged null record;
        procedure Act (Self : Root) is abstract;
        type Child is abstract new Root with null record;
        not overriding procedure Act (Self : Child) is abstract;
        overriding procedure Other (Self : Child) is abstract;
        type Concrete is new Root with null record;
        overriding procedure Act (Self : Concrete) is abstract;
        type Equal_Root is abstract tagged null record;
        function "=" (Left, Right : Equal_Root) return Boolean is abstract;
        type Equal_Child is new Equal_Root with null record;
    end Model;
begin
    null;
end AbstractOverrideErrors;
