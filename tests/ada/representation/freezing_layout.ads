package Freezing_Layout is
    type Item is record
        First, Second : Integer;
    end record;
    subtype Item_View is Item;
    type Item_Pointer is access Item;
    type Factory is access function return Item_View;
    function Make return Item_View;
    function Sum (Value : Item_View := Make) return Integer;
    function Chosen (Value : Integer := Sum) return Integer;
    function Explicit_Value (Value : Integer := Chosen) return Integer;
    pragma Import (C, Explicit_Value, "abs");
    Explicit_Result : constant Integer := Explicit_Value (-7);
private
    Slot : constant := 4;
    for Item use record
        First  at Slot range 0 .. 31;
        Second at 0 range 0 .. 31;
    end record;
    for Item'Alignment use 8;
    for Item'Size use 128;
end Freezing_Layout;
