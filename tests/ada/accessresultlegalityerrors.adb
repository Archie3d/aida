procedure AccessResultLegalityErrors is
    package Model is
        type Root is tagged null record;
        function Make return access Root;
        type Missing is new Root with null record;
        type Middle is abstract new Root with null record;
        type Missing_Again is new Middle with null record;
        type Leaf is new Root with null record;
        overriding function Make return not null access Leaf;
    end Model;
    package body Model is
        function Make return access Root is
        begin
            return null;
        end Make;
        overriding function Make return not null access Leaf is
        begin
            raise Program_Error;
        end Make;
    end Model;
begin
    null;
end AccessResultLegalityErrors;
