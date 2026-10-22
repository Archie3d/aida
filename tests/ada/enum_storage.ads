package Enum_Storage is
    type Code is (First, Last);
    for Code use (5, 70);
    Global : Code := Last;
    type Base_Code is (First, Last);
    for Base_Code use (-7, 250);
    function Copy (Item : Base_Code) return Base_Code;
    type Compact_Code is new Base_Code;
    for Compact_Code use (200, 255);
    type Wide_Code is new Base_Code;
    for Wide_Code use (4, 250);
    for Wide_Code'Size use 32;
    for Wide_Code'Alignment use 4;
end Enum_Storage;
