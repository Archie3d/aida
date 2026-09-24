procedure Generic_Contract_Errors is
    generic
        type Item is private;
    function Add (L, R : Item) return Item;
    function Add (L, R : Item) return Item is
    begin
        return L + R;
    end Add;

    generic
        type Index_Type is (<>);
    function Step (X : Index_Type) return Index_Type;
    function Step (X : Index_Type) return Index_Type is
    begin
        return X + 1;
    end Step;

    generic
        type Left_Type is private;
        type Right_Type is private;
    procedure Mix (L : in out Left_Type; R : Right_Type);
    procedure Mix (L : in out Left_Type; R : Right_Type) is
    begin
        L := R;
    end Mix;

    generic
        type Item is private;
    function Less (L, R : Item) return Boolean;
    function Less (L, R : Item) return Boolean is
    begin
        return L < R;
    end Less;

    generic
        type Item is limited private;
    procedure Copy (L : in out Item; R : Item);
    procedure Copy (L : in out Item; R : Item) is
    begin
        L := R;
    end Copy;
begin
    null;
end Generic_Contract_Errors;
