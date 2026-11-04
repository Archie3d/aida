package Freezing_Layout is
    type Item is record
        First, Second : Integer;
    end record;
    subtype Item_View is Item;
    type Item_Pointer is access Item;
    function Make return Item_View;
private
    Slot : constant := 4;
    for Item use record
        First  at Slot range 0 .. 31;
        Second at 0 range 0 .. 31;
    end record;
    for Item'Alignment use 8;
    for Item'Size use 128;
end Freezing_Layout;
