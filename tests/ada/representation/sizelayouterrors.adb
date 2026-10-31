procedure SizeLayoutErrors is
    type Single is digits 6;
    for Single'Size use 8;
    for Single'Size use 64;
    type Double is digits 15;
    for Double'Size use 32;
    type Fixed is delta 0.125 range -10.0 .. 10.0;
    for Fixed'Size use 32;
    type Pointer is access Integer;
    for Pointer'Size use 32;
    type Color is (Red, Blue);
    for Color'Size use 64;
    type Pair is record
        A, B : Integer;
    end record;
    for Pair'Size use 8;
    for Pair'Size use 128;
    type Values is array (1 .. 4) of Integer;
    for Values'Size use 64;
    type Open_Values is array (Integer range <>) of Integer;
    for Open_Values'Size use 64;
    subtype Small is Integer range -128 .. 127;
    for Small'Size use 8;
    type Incomplete;
    for Incomplete'Size use 64;
    type Incomplete is record
        Value : Integer;
    end record;
    type Late is range -128 .. 127;
    subtype Cached is Late;
    for Late'Size use 8;
    type Field_Type is range -128 .. 127;
    type Container is record
        Value : Field_Type;
        Other : Integer;
    end record;
    for Field_Type'Size use 64;
    type Self_Base is range -128 .. 127;
    for Self_Base'Size use Self_Base'Base'Size * 2;
begin
    null;
end SizeLayoutErrors;
