procedure AlignmentErrors is
    Dynamic : Integer := 4;
    type Negative is range 0 .. 10;
    for Negative'Alignment use -1;
    type Odd is range 0 .. 10;
    for Odd'Alignment use 3;
    type Large is range 0 .. 10;
    for Large'Alignment use 16;
    type Nonstatic is range 0 .. 10;
    for Nonstatic'Alignment use Dynamic;
    type Real_Value is range 0 .. 10;
    for Real_Value'Alignment use 4.0;
    type Weak is range 0 .. 10;
    for Weak'Alignment use 1;
    type Duplicate is range 0 .. 10;
    for Duplicate'Alignment use 4;
    for Duplicate'Alignment use 4;
    type Late is range 0 .. 10;
    Object : Late;
    for Late'Alignment use 4;
    subtype Sub is Integer range 0 .. 10;
    for Sub'Alignment use 4;
    type Too_Small is range 0 .. 10;
    for Too_Small'Alignment use 8;
    type Byte is range 0 .. 255;
    for Byte'Size use 8;
    type Bytes is array (1 .. 3) of Byte;
    for Bytes'Alignment use 8;
    type Tagged_Value is tagged record
        X : Integer;
    end record;
    for Tagged_Value'Alignment use 8;
    type Incomplete;
    for Incomplete'Alignment use 4;
    type Incomplete is range 0 .. 10;
    for Object'Alignment use 4;
    type Size_Order is range 0 .. 100;
    for Size_Order'Size use 64;
    for Size_Order'Alignment use 8;
    for Size_Order'Size use 32;
    type Record_Order is record
        X : Byte;
    end record;
    for Record_Order'Size use 8;
    for Record_Order'Alignment use 8;
    Bad_Attribute : Integer := Integer'Alignment (1);
begin
    null;
end AlignmentErrors;
