procedure PrimitivePlacementErrors is
    package P is
        type Root is tagged null record;
        procedure Read (X : Root);
        Object : Root;
        procedure Late (X : Root);
    end P;
    package body P is
        procedure Read (X : Root) is
        begin
            null;
        end Read;
        procedure Late (X : Root) is
        begin
            null;
        end Late;
    end P;
    type Child is new P.Root with null record;
    procedure Helper is
    begin
        null;
    end Helper;
    overriding procedure Read (X : Child) is
    begin
        null;
    end Read;
    package Alias_Parent is
        type Root is tagged null record;
        subtype Alias is Root;
        type Child is new Alias with null record;
        procedure Late (X : Root);
    end Alias_Parent;
    package body Alias_Parent is
        procedure Late (X : Root) is
        begin
            null;
        end Late;
    end Alias_Parent;
begin
    null;
end PrimitivePlacementErrors;
