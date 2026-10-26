procedure GenericPrivateErrors is
    type Vector is array (Integer range <>) of Integer;
    type Record_Type (N : Integer) is null record;
    type Limited_Type is limited null record;
    generic
        type T is private;
    package Definite is
    end Definite;
    generic
        type T (<>) is private;
    package Indefinite is
    end Indefinite;
    package Bad_Array is new Definite (Vector);
    package Bad_Record is new Definite (Record_Type);
    package Bad_Limited is new Indefinite (Limited_Type);
    generic
        type T (<>) is private;
    package Invalid is
        Object : T;
        type Array_Type is array (1 .. 2) of T;
        type Composite is record
            Field : T;
        end record;
    end Invalid;
    generic
        type T (<>) is private;
    package Wrong_Nesting is
        package Bad_Actual is new Definite (T);
    end Wrong_Nesting;
    subtype Fixed is Vector (1 .. 3);
    subtype Fixed_Record is Record_Type (4);
    package Good_Array is new Definite (Fixed);
    package Good_Record is new Definite (Fixed_Record);
    package Good_Indefinite is new Indefinite (Vector);
begin
    null;
end GenericPrivateErrors;
