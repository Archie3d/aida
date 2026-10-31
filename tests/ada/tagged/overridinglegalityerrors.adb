procedure OverridingLegalityErrors is
    overriding procedure Local is null;
    not overriding procedure Other is null;
    package Model is
        type Root is tagged null record;
        procedure Act (Self : Root) is null;
        type Child is new Root with null record;
        overriding procedure Act (Self : Child);
        type Hidden is tagged private;
        overriding procedure Act (Self : Hidden);
        type Late is tagged private;
        not overriding procedure Act (Self : Late);
        type Late_Null is tagged private;
        not overriding procedure Act (Self : Late_Null) is null;
    private
        type Late_Null is new Root with null record;
        type Hidden is new Root with null record;
        type Late is new Root with null record;
    end Model;
    package body Model is
        not overriding procedure Act (Self : Child) is null;
        procedure Act (Self : Hidden) is null;
        procedure Act (Self : Late) is null;
    end Model;
    generic
        type Item is private;
    procedure Template (Self : Item);
    procedure Template (Self : Item) is
    begin
        null;
    end Template;
    not overriding procedure Instance is new Template (Integer);
    package Instances is
        type Root is tagged null record;
        procedure Act (Self : Root) is null;
        type Child is new Root with null record;
        not overriding procedure Act is new Template (Child);
        overriding procedure Missing is new Template (Child);
        procedure Act is new Template (Child);
    end Instances;
begin
    null;
end OverridingLegalityErrors;
