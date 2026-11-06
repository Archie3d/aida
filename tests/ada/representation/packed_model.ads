package Packed_Model is
    type Bits is array (Integer range <>) of Boolean;
    pragma Pack (Bits);
    subtype Eleven is Bits (-3 .. 7);
    type Holder is record
        Flags : Eleven := (others => False);
    end record;
    function Invert (Value : Bits) return Bits;
    procedure Toggle (Value : in out Bits);
end Packed_Model;
