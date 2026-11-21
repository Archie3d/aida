with Ada.Text_IO;
with Interfaces;
with Interfaces.C;
with Interfaces_Pure;

procedure InterfacesScalars is
    package I renames Interfaces;
    package C renames Interfaces.C;
    use type I.Unsigned_8;
    use type C.size_t;
    use type C.int;
    use type C.double;
    use type C.char;
    use type C.wchar_t;
    use type C.char16_t;
    use type C.char32_t;

    generic
        type Word is mod <>;
        Bits : Positive;
        with function Left (Value : Word; Amount : Natural) return Word;
        with function Right (Value : Word; Amount : Natural) return Word;
        with function Arithmetic (Value : Word; Amount : Natural) return Word;
        with function Rotate_Left (Value : Word; Amount : Natural) return Word;
        with function Rotate_Right (Value : Word; Amount : Natural) return Word;
    procedure Check_Shifts;

    procedure Check_Shifts is
        High : constant Word := Word'Last / 2 + 1;
        Mixed : constant Word := High + 1;
        Count : Natural := Bits;
    begin
        if Left (Mixed, 1) /= 2 or else Right (Mixed, 1) /= High / 2
            or else Arithmetic (Mixed, 1) /= High + High / 2
            or else Left (Mixed, 0) /= Mixed or else Right (Mixed, 0) /= Mixed
            or else Left (Mixed, Bits - 1) /= High or else Right (Mixed, Bits - 1) /= 1
            or else Left (Mixed, Count) /= 0 or else Right (Mixed, Count) /= 0
            or else Arithmetic (Mixed, Count) /= Word'Last or else Arithmetic (1, Count) /= 0
            or else Rotate_Left (Mixed, 1) /= 3 or else Rotate_Right (Mixed, 1) /= High + High / 2
            or else Rotate_Left (Mixed, 0) /= Mixed or else Rotate_Right (Mixed, Count) /= Mixed
            or else Rotate_Left (Mixed, Natural'Last) /= High + High / 2
            or else Rotate_Right (Mixed, Natural'Last) /= 3
        then
            raise Program_Error;
        end if;
    end Check_Shifts;

    procedure Check_8 is new Check_Shifts
        (I.Unsigned_8, 8, I.Shift_Left, I.Shift_Right, I.Shift_Right_Arithmetic, I.Rotate_Left, I.Rotate_Right);
    procedure Check_16 is new Check_Shifts
        (I.Unsigned_16, 16, I.Shift_Left, I.Shift_Right, I.Shift_Right_Arithmetic, I.Rotate_Left, I.Rotate_Right);
    procedure Check_32 is new Check_Shifts
        (I.Unsigned_32, 32, I.Shift_Left, I.Shift_Right, I.Shift_Right_Arithmetic, I.Rotate_Left, I.Rotate_Right);
    procedure Check_64 is new Check_Shifts
        (I.Unsigned_64, 64, I.Shift_Left, I.Shift_Right, I.Shift_Right_Arithmetic, I.Rotate_Left, I.Rotate_Right);

    function Is_Signed (Value : C.signed_char) return Integer is
    begin
        return 1;
    end Is_Signed;
    function Is_Signed (Value : C.unsigned_char) return Integer is
    begin
        return 0;
    end Is_Signed;

    function Native_Widths (Long_Bits, Wchar_Bits, Size_Bits, Ptrdiff_Bits,
                            Char16_Bits, Char32_Bits, C_Float_Bits, Double_Bits,
                            Plain_Char_Signed : Integer) return Integer;
    pragma Import (C, Native_Widths, "aida_interfaces_widths");
    function Native_Integers (S : C.short; W : C.int; L : C.long; U : C.unsigned_long;
                              Length : C.size_t; Difference : C.ptrdiff_t;
                              B : C.signed_char; UB : C.unsigned_char) return C.int;
    pragma Import (C, Native_Integers, "aida_interfaces_integers");
    function Native_Char (Value : C.char) return C.char;
    pragma Import (C, Native_Char, "aida_interfaces_char");
    function Native_Wchar (Value : C.wchar_t) return C.wchar_t;
    pragma Import (C, Native_Wchar, "aida_interfaces_wchar");
    function Native_Char16 (Value : C.char16_t) return C.char16_t;
    pragma Import (C, Native_Char16, "aida_interfaces_char16");
    function Native_Char32 (Value : C.char32_t) return C.char32_t;
    pragma Import (C, Native_Char32, "aida_interfaces_char32");
    function Native_Float (Single : C.C_float; Wide : C.double) return C.double;
    pragma Import (C, Native_Float, "aida_interfaces_float");

    type Char_Function is access function (Value : C.char) return C.char;
    pragma Convention (C, Char_Function);
    Indirect_Char : Char_Function := Native_Char'Access;
    Invalid_Count : Integer := -1;
    B : I.Unsigned_8 := 1;
    Invalid_Character : C.char32_t;
    Pure_Count : Interfaces_Pure.Native_Count := C.size_t (Interfaces_Pure.Double (3, 1));
begin
    Check_8;
    Check_16;
    Check_32;
    Check_64;

    if I.Integer_8'Size /= 8 or else I.Integer_16'Size /= 16
        or else I.Integer_32'Size /= 32 or else I.Integer_64'Size /= 64
        or else I.Unsigned_8'Size /= 8 or else I.Unsigned_16'Size /= 16
        or else I.Unsigned_32'Size /= 32 or else I.Unsigned_64'Size /= 64
        or else I.IEEE_Float_32'Machine_Mantissa /= 24 or else I.IEEE_Float_64'Machine_Mantissa /= 53
    then
        raise Program_Error;
    end if;
    if Pure_Count /= 6 or else Interfaces_Pure.Bits_Per_Byte /= 8
        or else C.CHAR_BIT /= 8 or else C.SCHAR_MIN /= -128 or else C.SCHAR_MAX /= 127 or else C.UCHAR_MAX /= 255
        or else Native_Widths (C.long'Size, C.wchar_t'Size, C.size_t'Size, C.ptrdiff_t'Size,
                               C.char16_t'Size, C.char32_t'Size, C.C_float'Size, C.double'Size,
                               Is_Signed (C.plain_char'First)) /= 1
        or else Native_Integers (C.short'First, C.int'First, C.long'First, C.unsigned_long'Last,
                                 C.size_t'Last, C.ptrdiff_t'First, C.signed_char'First, C.unsigned_char'Last) /= 1
        or else C.char'Pos (C.nul) /= 0 or else C.wchar_t'Pos (C.wide_nul) /= 0
        or else C.char16_t'Pos (C.char16_nul) /= 0 or else C.char32_t'Pos (C.char32_nul) /= 0
        or else Native_Char (C.char'Val (16#FF#)) /= C.char'Val (16#9A#)
        or else Indirect_Char (C.char'Val (16#FF#)) /= C.char'Val (16#9A#)
        or else Native_Wchar (C.wchar_t'Val (16#FEDC#)) /= C.wchar_t'Val (16#FEFE#)
        or else Native_Char16 (C.char16_t'Val (16#FEFF#)) /= C.char16_t'Val (16#FDFF#)
        or else Native_Char32 (C.char32_t'Val (16#10FFFE#)) /= C.char32_t'Val (16#10FFFF#)
        or else Native_Float (1.25, -2.5) /= -1.25
    then
        raise Program_Error;
    end if;
    begin
        B := I.Shift_Left (Amount => Invalid_Count, Value => B);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    begin
        Invalid_Character := Native_Char32 (C.char32_nul);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    Ada.Text_IO.Put_Line ("Interfaces scalar types passed");
end InterfacesScalars;
