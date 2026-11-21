with Ada.Text_IO;
with System;

procedure CScalarWidth is
    type Signed_Byte is range -128 .. 127;
    for Signed_Byte'Size use 8;
    type Unsigned_Byte is mod 2 ** 8;
    for Unsigned_Byte'Size use 8;
    type Signed_Half is range -32768 .. 32767;
    for Signed_Half'Size use 16;
    type Unsigned_Half is mod 2 ** 16;
    for Unsigned_Half'Size use 16;
    type Signed_Word is range -2 ** 31 .. 2 ** 31 - 1;
    type Unsigned_Word is mod 2 ** 32;
    type Explicit_Word is mod 2 ** 32;
    for Explicit_Word'Size use 32;
    type Signed_Quad is range -2 ** 63 .. 2 ** 63 - 1;
    type Unsigned_Quad is mod 2 ** 64;

    function Byte_Value (Value : Signed_Byte) return Signed_Byte;
    pragma Import (C, Byte_Value, "aida_abi_signed_byte");
    function Unsigned_Byte_Value (Value : Unsigned_Byte) return Unsigned_Byte;
    pragma Import (C, Unsigned_Byte_Value, "aida_abi_unsigned_byte");
    function Half_Value (Value : Signed_Half) return Signed_Half;
    pragma Import (C, Half_Value, "aida_abi_signed_half");
    function Unsigned_Half_Value (Value : Unsigned_Half) return Unsigned_Half;
    pragma Import (C, Unsigned_Half_Value, "aida_abi_unsigned_half");
    function Word_Value (Value : Signed_Word) return Signed_Word;
    pragma Import (C, Word_Value, "aida_abi_signed_word");
    function Unsigned_Word_Value (Value : Unsigned_Word) return Unsigned_Word;
    pragma Import (C, Unsigned_Word_Value, "aida_abi_unsigned_word");
    function Quad_Value (Value : Signed_Quad) return Signed_Quad;
    pragma Import (C, Quad_Value, "aida_abi_signed_quad");
    function Unsigned_Quad_Value (Value : Unsigned_Quad) return Unsigned_Quad;
    pragma Import (C, Unsigned_Quad_Value, "aida_abi_unsigned_quad");
    function Mixed (A, B, C, D, E, F, G, H : Signed_Word;
                    I : Signed_Byte; J : Unsigned_Byte; K : Signed_Half;
                    L : Unsigned_Half; M : Unsigned_Word) return Unsigned_Word;
    pragma Import (C, Mixed, "aida_abi_mixed_words");
    function Check_Layout (Words, Narrow, Wide, Widest : System.Address) return Integer;
    pragma Import (C, Check_Layout, "aida_abi_update_layout");

    type Byte_Call is access function (Value : Signed_Byte) return Signed_Byte;
    pragma Convention (C, Byte_Call);
    type Word_Call is access function (Value : Unsigned_Word) return Unsigned_Word;
    pragma Convention (C, Word_Call);
    Byte_Indirect : Byte_Call := Byte_Value'Access;
    Word_Indirect : Word_Call := Unsigned_Word_Value'Access;

    type Word_Array is array (1 .. 3) of Unsigned_Word;
    type Narrow_Array is array (Unsigned_Quad range <>) of aliased Character;
    pragma Pack (Narrow_Array);
    for Narrow_Array'Component_Size use 8;
    type Wide_Array is array (1 .. 2) of aliased Wide_Character;
    pragma Pack (Wide_Array);
    for Wide_Array'Component_Size use 16;
    type Widest_Array is array (1 .. 2) of aliased Wide_Wide_Character;
    pragma Pack (Widest_Array);
    for Widest_Array'Component_Size use 32;
    type Char_Access is access all Character;

    Words : Word_Array := (Unsigned_Word'Last, 16#8000_0000#, 17);
    Narrow : Narrow_Array (Unsigned_Quad'Last - 1 .. Unsigned_Quad'Last) :=
        (Character'Val (16#FF#), Character'Val (16#98#));
    Wide : Wide_Array := (Wide_Character'Val (16#FEFF#), Wide_Character'Val (16#1234#));
    Widest : Widest_Array := (Wide_Wide_Character'Val (16#10_FFFE#), Wide_Wide_Character'Val (16#1_0000#));
    Element : Char_Access := Narrow (Unsigned_Quad'Last)'Access;
    Atomic_Word : Unsigned_Word := Unsigned_Word'Last;
    pragma Atomic (Atomic_Word);
    Signed_Result : Signed_Word;
begin
    if Signed_Byte'Size /= 8 or else Unsigned_Byte'Size /= 8
        or else Signed_Half'Size /= 16 or else Unsigned_Half'Size /= 16
        or else Signed_Word'Size /= 32 or else Unsigned_Word'Size /= 32
        or else Explicit_Word'Size /= 32
        or else Signed_Quad'Size /= 64 or else Unsigned_Quad'Size /= 64
        or else Unsigned_Word'Alignment /= 4 or else Words'Size /= 96
        or else Narrow'Size /= 16 or else Wide'Size /= 32 or else Widest'Size /= 64
    then
        raise Program_Error;
    end if;
    if Byte_Value (Signed_Byte'First) /= -125 or else Byte_Indirect (Signed_Byte'First) /= -125
        or else Unsigned_Byte_Value (Unsigned_Byte'Last) /= 253
        or else Half_Value (Signed_Half'First) /= -32765 or else Unsigned_Half_Value (Unsigned_Half'Last) /= 65533
        or else Word_Value (Signed_Word'First) /= Signed_Word'First + 1
        or else Unsigned_Word_Value (Unsigned_Word'Last) /= Unsigned_Word'Last - 1
        or else Word_Indirect (Unsigned_Word'Last) /= Unsigned_Word'Last - 1
        or else Quad_Value (Signed_Quad'First) /= Signed_Quad'First + 1
        or else Unsigned_Quad_Value (Unsigned_Quad'Last) /= Unsigned_Quad'Last - 1
        or else Mixed (1, 2, 3, 4, 5, 6, 7, 8, Signed_Byte'First, Unsigned_Byte'Last,
                       Signed_Half'First, Unsigned_Half'Last, Unsigned_Word'Last) /= 16#8000_0000#
    then
        raise Program_Error;
    end if;
    if Check_Layout (Words (1)'Address, Narrow (Unsigned_Quad'Last - 1)'Address,
                     Wide (1)'Address, Widest (1)'Address) /= 1
        or else Element.all /= Character'Val (16#AB#) or else Wide (2) /= Wide_Character'Val (16#FFFE#)
        or else Widest (2) /= Wide_Wide_Character'Val (16#10_FFFF#)
        or else Words (2) /= 16#FEDC_BA98# or else Long_Integer (Words (2)) /= 16#FEDC_BA98#
        or else Atomic_Word /= Unsigned_Word'Last or else Atomic_Word + 1 /= 0
    then
        raise Program_Error;
    end if;
    begin
        Signed_Result := Signed_Word (Words (1));
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    Ada.Text_IO.Put_Line ("native scalar widths passed");
end CScalarWidth;
