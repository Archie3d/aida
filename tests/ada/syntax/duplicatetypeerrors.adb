procedure DuplicateTypeErrors is
    S : String (1 .. 20) := (others => ' ');
    type S is record
        Field : Integer;
    end record;

    type Count is range 1 .. 10;
    type cOuNt is range 1 .. 20;

    subtype Index is Integer;
    type Index is range 1 .. 10;

    Item : Integer;
    type Item;

    Value : Integer;
    subtype Value is Integer;

    subtype Position is Integer;
    subtype Position is Integer;

    procedure Action is null;
    type Action is record
        Field : Integer;
    end record;
begin
    null;
end DuplicateTypeErrors;
