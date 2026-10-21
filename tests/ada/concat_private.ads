package Concat_Private is
    type Item is private;
    type Items is array (Positive range <>) of Item;
    function Make (Value : Integer) return Item;
    function Number (Value : Item) return Integer;
private
    type Item is record
        Value : Integer;
    end record;
end Concat_Private;
