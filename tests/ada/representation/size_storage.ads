package Size_Storage is
    subtype Byte is Long_Integer range -128 .. 127;
    for Byte'Size use 8;
    subtype Wide is Integer range -128 .. 127;
    for Wide'Size use 64;
    type Pair is record
        A, B : Integer;
    end record;
    for Pair'Size use 128;
    function Copy (Item : Pair) return Pair;
    type Derived is new Pair;
    for Derived'Size use 512;
    subtype Roomy is Pair;
    for Roomy'Size use 256;
    type Pair_Array is array (1 .. 3) of Roomy;
    for Pair_Array'Size use 1024;
    subtype Short_Array is Pair_Array;
    for Short_Array'Size use 768;
    Global : Pair_Array := (others => (A => 5, B => 6));
end Size_Storage;
