with System;
with Ada.Unchecked_Conversion;
procedure PackedSignedErrors is
    type Small is range -7 .. 7;
    type Values is array (1 .. 3) of Small;
    pragma Pack (Values);
    type Modular_Value is mod 16;
    type Modular_Array is array (1 .. 3) of Modular_Value;
    pragma Pack (Modular_Array);
    type Aliased_Array is array (1 .. 3) of aliased Small;
    pragma Pack (Aliased_Array);
    a : Values := (others => -7);
    slice : Values := a (1 .. 3);
    joined : Values := a & a;
    address : System.Address := a (1)'Address;
    type Tiny is range -1 .. 0;
    type Tiny_Array is array (1 .. 3) of Tiny;
    pragma Pack (Tiny_Array);
    function Convert is new Ada.Unchecked_Conversion (Tiny_Array, Integer);
    type Raw is array (1 .. 3) of Small;
    unpacked : Raw := Raw (a);
begin
    null;
end PackedSignedErrors;
