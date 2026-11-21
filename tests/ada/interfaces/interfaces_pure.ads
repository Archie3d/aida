with Interfaces;
with Interfaces.C;

package Interfaces_Pure is
    pragma Pure;
    subtype Native_Count is Interfaces.C.size_t;
    Bits_Per_Byte : constant := Interfaces.C.CHAR_BIT;
    function Double (Value : Interfaces.Unsigned_64; Amount : Natural) return Interfaces.Unsigned_64
        renames Interfaces.Shift_Left;
end Interfaces_Pure;
