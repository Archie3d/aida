procedure RepresentationErrors is
    type Value is range 0 .. 100;
    for Value'Alignment use 16;
    for Value'Unknown_Attribute use 8;
    type Color is (Red, Blue);
    for Color use (Red => 1, Blue => 3);
    type Pair is record
        A, B : Integer;
    end record;
    for Pair use record
        A at 0 range 0 .. 31;
        B at 4 range 0 .. 31;
    end record;
    Object : Integer;
    for Object'Address use 0;
    for Object use at 0;
    for Value'Bit_Order use 0;
begin
    null;
end RepresentationErrors;
