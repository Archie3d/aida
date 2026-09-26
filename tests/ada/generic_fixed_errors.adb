procedure Generic_Fixed_Errors is
    generic
        type Item is delta <>;
    package Fixed_Only is
    end Fixed_Only;
    type Choice is (A, B);
    type Record_Type is record
        X : Integer;
    end record;
    package Bad_Integer is new Fixed_Only (Integer);
    package Bad_Float is new Fixed_Only (Float);
    package Bad_Enum is new Fixed_Only (Choice);
    package Bad_Record is new Fixed_Only (Record_Type);

    generic
        type Item is delta <>;
    function Bad_Mod (L, R : Item) return Item;
    function Bad_Mod (L, R : Item) return Item is
    begin
        return L mod R;
    end Bad_Mod;

    generic
        type Item is delta <>;
    function Bad_Power (X : Item) return Item;
    function Bad_Power (X : Item) return Item is
    begin
        return X ** 2;
    end Bad_Power;

    generic
        type Left_Type is delta <>;
        type Right_Type is delta <>;
    procedure Mix (L : in out Left_Type; R : Right_Type);
    procedure Mix (L : in out Left_Type; R : Right_Type) is
    begin
        L := R;
    end Mix;
begin
    null;
end Generic_Fixed_Errors;
