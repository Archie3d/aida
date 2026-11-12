package Packed_Signed_Model is
    type Small is range -7 .. 7;
    type Values is array (Integer range <>) of Small;
    pragma Pack (Values);
    type Holder is record
        items : Values (-3 .. 7) := (others => -3);
    end record;
    function Copy (item : Values) return Values;
    procedure Negate (item : in out Values);
end Packed_Signed_Model;
