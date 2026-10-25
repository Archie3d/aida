procedure InterfaceConversionErrors is
    package Model is
        type Root is interface;
        function Read (Self : Root) return Integer is abstract;
        type Other is interface;
        type Base is tagged null record;
        type Item is new Root with null record;
        overriding function Read (Self : Item) return Integer;
    end Model;
    package body Model is
        overriding function Read (Self : Item) return Integer is
        begin
            return 1;
        end Read;
    end Model;
    Object : aliased Model.Item;
    Wide : Model.Root'Class := Object;
    Allowed : Model.Base'Class := Model.Base'Class (Wide);
    Bad_Specific : Model.Base := Model.Base (Wide);
    Bad_Interface : Model.Other := Model.Other (Wide);
    type Source is access all Model.Root'Class;
    type Target is access all Model.Base;
    Pointer : Source := Object'Access;
    Bad_Access : Target := Target (Pointer);
begin
    if Wide in Model.Base then
        null;
    end if;
end InterfaceConversionErrors;
