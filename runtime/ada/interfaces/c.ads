package Interfaces.C is
    pragma Pure;

    CHAR_BIT : constant := 8;
    SCHAR_MIN : constant := -128;
    SCHAR_MAX : constant := 127;
    UCHAR_MAX : constant := 255;

    type int is new Integer_32;
    type short is new Integer_16;
    -- These subtype marks are supplied by the compiler from the target C ABI.
    type long is new Aida_C_Long;
    type signed_char is new Integer_8;
    type unsigned is new Unsigned_32;
    type unsigned_short is new Unsigned_16;
    type unsigned_long is mod 2 ** long'Size;
    type unsigned_char is new Unsigned_8;
    subtype plain_char is Aida_C_Plain_Char;

    type ptrdiff_t is new Integer_64;
    type size_t is new Unsigned_64;

    type C_float is new IEEE_Float_32;
    type double is new IEEE_Float_64;
    -- The compiler makes this distinct type available on binary64 targets
    -- with the C double calling convention.
    type long_double is new IEEE_Float_64;

    type char is new Character;
    type wchar_t is new Aida_C_Wchar;
    type char16_t is new Wide_Character;
    type char32_t is new Wide_Wide_Character;

    nul : constant char := char'Val (0);
    wide_nul : constant wchar_t := wchar_t'Val (0);
    char16_nul : constant char16_t := char16_t'Val (0);
    char32_nul : constant char32_t := char32_t'Val (0);
end Interfaces.C;
