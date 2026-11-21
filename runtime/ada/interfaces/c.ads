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

    type char_array is array (size_t range <>) of aliased char;
    pragma Pack (char_array);
    for char_array'Component_Size use CHAR_BIT;

    function To_C (Item : Character) return char;
    function To_Ada (Item : char) return Character;
    function Is_Nul_Terminated (Item : char_array) return Boolean;
    function To_C (Item : String; Append_Nul : Boolean := True) return char_array;
    function To_Ada (Item : char_array; Trim_Nul : Boolean := True) return String;
    procedure To_C (Item : String; Target : out char_array; Count : out size_t;
                    Append_Nul : Boolean := True);
    procedure To_Ada (Item : char_array; Target : out String; Count : out Natural;
                      Trim_Nul : Boolean := True);

    type wchar_array is array (size_t range <>) of aliased wchar_t;
    pragma Pack (wchar_array);

    function To_C (Item : Wide_Character) return wchar_t;
    function To_Ada (Item : wchar_t) return Wide_Character;
    function Is_Nul_Terminated (Item : wchar_array) return Boolean;
    function To_C (Item : Wide_String; Append_Nul : Boolean := True) return wchar_array;
    function To_Ada (Item : wchar_array; Trim_Nul : Boolean := True) return Wide_String;
    procedure To_C (Item : Wide_String; Target : out wchar_array; Count : out size_t;
                    Append_Nul : Boolean := True);
    procedure To_Ada (Item : wchar_array; Target : out Wide_String; Count : out Natural;
                      Trim_Nul : Boolean := True);

    type char16_array is array (size_t range <>) of aliased char16_t;
    pragma Pack (char16_array);

    function To_C (Item : Wide_Character) return char16_t;
    function To_Ada (Item : char16_t) return Wide_Character;
    function Is_Nul_Terminated (Item : char16_array) return Boolean;
    function To_C (Item : Wide_String; Append_Nul : Boolean := True) return char16_array;
    function To_Ada (Item : char16_array; Trim_Nul : Boolean := True) return Wide_String;
    procedure To_C (Item : Wide_String; Target : out char16_array; Count : out size_t;
                    Append_Nul : Boolean := True);
    procedure To_Ada (Item : char16_array; Target : out Wide_String; Count : out Natural;
                      Trim_Nul : Boolean := True);

    type char32_array is array (size_t range <>) of aliased char32_t;
    pragma Pack (char32_array);

    function To_C (Item : Wide_Wide_Character) return char32_t;
    function To_Ada (Item : char32_t) return Wide_Wide_Character;
    function Is_Nul_Terminated (Item : char32_array) return Boolean;
    function To_C (Item : Wide_Wide_String; Append_Nul : Boolean := True) return char32_array;
    function To_Ada (Item : char32_array; Trim_Nul : Boolean := True) return Wide_Wide_String;
    procedure To_C (Item : Wide_Wide_String; Target : out char32_array; Count : out size_t;
                    Append_Nul : Boolean := True);
    procedure To_Ada (Item : char32_array; Target : out Wide_Wide_String; Count : out Natural;
                      Trim_Nul : Boolean := True);

    Terminator_Error : exception;
end Interfaces.C;
