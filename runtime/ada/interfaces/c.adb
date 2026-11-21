package body Interfaces.C is
    function To_C (Item : Character) return char is
    begin
        return char'Val (Character'Pos (Item));
    end To_C;

    function To_Ada (Item : char) return Character is
    begin
        return Character'Val (char'Pos (Item));
    end To_Ada;

    function To_C (Item : Wide_Character) return wchar_t is
    begin
        return wchar_t'Val (Wide_Character'Pos (Item));
    end To_C;

    function To_Ada (Item : wchar_t) return Wide_Character is
    begin
        return Wide_Character'Val (wchar_t'Pos (Item));
    end To_Ada;

    function To_C (Item : Wide_Character) return char16_t is
    begin
        return char16_t'Val (Wide_Character'Pos (Item));
    end To_C;

    function To_Ada (Item : char16_t) return Wide_Character is
    begin
        return Wide_Character'Val (char16_t'Pos (Item));
    end To_Ada;

    function To_C (Item : Wide_Wide_Character) return char32_t is
    begin
        return char32_t'Val (Wide_Wide_Character'Pos (Item));
    end To_C;

    function To_Ada (Item : char32_t) return Wide_Wide_Character is
    begin
        return Wide_Wide_Character'Val (char32_t'Pos (Item));
    end To_Ada;

    generic
        type Ada_Char is (<>);
        type C_Char is (<>);
        type Ada_Text is array (Positive range <>) of Ada_Char;
        type C_Text is array (size_t range <>) of aliased C_Char;
        Nul_Value : C_Char;
        with function Map_To_C (Item : Ada_Char) return C_Char;
        with function Map_To_Ada (Item : C_Char) return Ada_Char;
    package Character_Conversions is
        function Is_Nul_Terminated (Item : C_Text) return Boolean;
        function To_C (Item : Ada_Text; Append_Nul : Boolean := True) return C_Text;
        function To_Ada (Item : C_Text; Trim_Nul : Boolean := True) return Ada_Text;
        procedure To_C (Item : Ada_Text; Target : out C_Text; Count : out size_t;
                        Append_Nul : Boolean := True);
        procedure To_Ada (Item : C_Text; Target : out Ada_Text; Count : out Natural;
                          Trim_Nul : Boolean := True);
    end Character_Conversions;

    package body Character_Conversions is
        function Nul_Offset (Item : C_Text) return size_t is
        begin
            for Index in Item'Range loop
                if Item (Index) = Nul_Value then
                    return Index - Item'First;
                end if;
            end loop;
            return Item'Length;
        end Nul_Offset;

        function Is_Nul_Terminated (Item : C_Text) return Boolean is
        begin
            return Nul_Offset (Item) /= Item'Length;
        end Is_Nul_Terminated;

        function C_Length (Item : Ada_Text; Append_Nul : Boolean) return size_t is
        begin
            if Append_Nul then
                return size_t (Item'Length) + 1;
            end if;
            return Item'Length;
        end C_Length;

        procedure To_C (Item : Ada_Text; Target : out C_Text; Count : out size_t;
                        Append_Nul : Boolean := True) is
            Length : constant size_t := C_Length (Item, Append_Nul);
            Offset : size_t := 0;
        begin
            if Length > Target'Length then
                raise Constraint_Error;
            end if;
            for Index in Item'Range loop
                Target (Target'First + Offset) := Map_To_C (Item (Index));
                Offset := Offset + 1;
            end loop;
            if Append_Nul then
                Target (Target'First + Offset) := Nul_Value;
            end if;
            Count := Length;
        end To_C;

        function To_C (Item : Ada_Text; Append_Nul : Boolean := True) return C_Text is
            Length : constant size_t := C_Length (Item, Append_Nul);
            Count : size_t;
        begin
            -- A null result cannot have lower bound zero with a modular index.
            if Length = 0 then
                raise Constraint_Error;
            end if;
            return Result : C_Text (0 .. Length - 1) do
                To_C (Item, Result, Count, Append_Nul);
            end return;
        end To_C;

        function Ada_Length (Item : C_Text; Trim_Nul : Boolean) return Natural is
            Offset : size_t;
        begin
            if not Trim_Nul then
                return Item'Length;
            end if;
            Offset := Nul_Offset (Item);
            if Offset = Item'Length then
                raise Terminator_Error;
            end if;
            return Natural (Offset);
        end Ada_Length;

        procedure Copy_To_Ada (Item : C_Text; Target : out Ada_Text; Length : Natural) is
        begin
            for Position in 1 .. Length loop
                Target (Target'First + (Position - 1)) :=
                    Map_To_Ada (Item (Item'First + size_t (Position - 1)));
            end loop;
        end Copy_To_Ada;

        procedure To_Ada (Item : C_Text; Target : out Ada_Text; Count : out Natural;
                          Trim_Nul : Boolean := True) is
            Length : constant Natural := Ada_Length (Item, Trim_Nul);
        begin
            if Length > Target'Length then
                raise Constraint_Error;
            end if;
            Copy_To_Ada (Item, Target, Length);
            Count := Length;
        end To_Ada;

        function To_Ada (Item : C_Text; Trim_Nul : Boolean := True) return Ada_Text is
            Length : constant Natural := Ada_Length (Item, Trim_Nul);
        begin
            return Result : Ada_Text (1 .. Length) do
                Copy_To_Ada (Item, Result, Length);
            end return;
        end To_Ada;
    end Character_Conversions;

    package Narrow_Conversions is new Character_Conversions
        (Character, char, String, char_array, nul, To_C, To_Ada);

    function Is_Nul_Terminated (Item : char_array) return Boolean
        renames Narrow_Conversions.Is_Nul_Terminated;
    function To_C (Item : String; Append_Nul : Boolean := True) return char_array
        renames Narrow_Conversions.To_C;
    function To_Ada (Item : char_array; Trim_Nul : Boolean := True) return String
        renames Narrow_Conversions.To_Ada;
    procedure To_C (Item : String; Target : out char_array; Count : out size_t;
                    Append_Nul : Boolean := True)
        renames Narrow_Conversions.To_C;
    procedure To_Ada (Item : char_array; Target : out String; Count : out Natural;
                      Trim_Nul : Boolean := True)
        renames Narrow_Conversions.To_Ada;

    package Wide_Conversions is new Character_Conversions
        (Wide_Character, wchar_t, Wide_String, wchar_array, wide_nul, To_C, To_Ada);

    function Is_Nul_Terminated (Item : wchar_array) return Boolean
        renames Wide_Conversions.Is_Nul_Terminated;
    function To_C (Item : Wide_String; Append_Nul : Boolean := True) return wchar_array
        renames Wide_Conversions.To_C;
    function To_Ada (Item : wchar_array; Trim_Nul : Boolean := True) return Wide_String
        renames Wide_Conversions.To_Ada;
    procedure To_C (Item : Wide_String; Target : out wchar_array; Count : out size_t;
                    Append_Nul : Boolean := True)
        renames Wide_Conversions.To_C;
    procedure To_Ada (Item : wchar_array; Target : out Wide_String; Count : out Natural;
                      Trim_Nul : Boolean := True)
        renames Wide_Conversions.To_Ada;

    package Char16_Conversions is new Character_Conversions
        (Wide_Character, char16_t, Wide_String, char16_array, char16_nul, To_C, To_Ada);

    function Is_Nul_Terminated (Item : char16_array) return Boolean
        renames Char16_Conversions.Is_Nul_Terminated;
    function To_C (Item : Wide_String; Append_Nul : Boolean := True) return char16_array
        renames Char16_Conversions.To_C;
    function To_Ada (Item : char16_array; Trim_Nul : Boolean := True) return Wide_String
        renames Char16_Conversions.To_Ada;
    procedure To_C (Item : Wide_String; Target : out char16_array; Count : out size_t;
                    Append_Nul : Boolean := True)
        renames Char16_Conversions.To_C;
    procedure To_Ada (Item : char16_array; Target : out Wide_String; Count : out Natural;
                      Trim_Nul : Boolean := True)
        renames Char16_Conversions.To_Ada;

    package Char32_Conversions is new Character_Conversions
        (Wide_Wide_Character, char32_t, Wide_Wide_String, char32_array, char32_nul, To_C, To_Ada);

    function Is_Nul_Terminated (Item : char32_array) return Boolean
        renames Char32_Conversions.Is_Nul_Terminated;
    function To_C (Item : Wide_Wide_String; Append_Nul : Boolean := True) return char32_array
        renames Char32_Conversions.To_C;
    function To_Ada (Item : char32_array; Trim_Nul : Boolean := True) return Wide_Wide_String
        renames Char32_Conversions.To_Ada;
    procedure To_C (Item : Wide_Wide_String; Target : out char32_array; Count : out size_t;
                    Append_Nul : Boolean := True)
        renames Char32_Conversions.To_C;
    procedure To_Ada (Item : char32_array; Target : out Wide_Wide_String; Count : out Natural;
                      Trim_Nul : Boolean := True)
        renames Char32_Conversions.To_Ada;

end Interfaces.C;
