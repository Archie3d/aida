with Ada.Text_IO;
with Interfaces.C;

procedure InterfacesCharacters is
    package C renames Interfaces.C;
    use type C.size_t;
    use type C.wchar_t;

    generic
        type Ada_Char is (<>);
        type C_Char is (<>);
        type Ada_Text is array (Positive range <>) of Ada_Char;
        type C_Text is array (C.size_t range <>) of aliased C_Char;
        Symbol : Ada_Char;
        Nul_Value : C_Char;
        with function To_C (Item : Ada_Char) return C_Char;
        with function To_Ada (Item : C_Char) return Ada_Char;
        with function Contains_Nul (Item : C_Text) return Boolean;
        with function Encode (Item : Ada_Text; Append_Nul : Boolean := True) return C_Text;
        with function Decode (Item : C_Text; Trim_Nul : Boolean := True) return Ada_Text;
        with procedure Encode (Item : Ada_Text; Target : out C_Text; Count : out C.size_t;
                               Append_Nul : Boolean := True);
        with procedure Decode (Item : C_Text; Target : out Ada_Text; Count : out Natural;
                               Trim_Nul : Boolean := True);
    procedure Check_Family;

    procedure Check_Family is
        Top : constant C.size_t := C.size_t'Last;
        Sentinel : constant C_Char := C_Char'Val (42);
        Ada_Sentinel : constant Ada_Char := Ada_Char'Val (42);
        Source : Ada_Text (7 .. 9) := (Symbol, Ada_Char'Val (0), Ada_Char'Val (65));
        At_Last : Ada_Text (Integer'Last .. Integer'Last) := (others => Symbol);
        Empty_Ada : Ada_Text (7 .. 6);
        Empty_C : C_Text (Top .. 0);
        Encoded : C_Text := Encode (Source);
        Raw : C_Text := Encode (Source, Append_Nul => False);
        Back : Ada_Text := Decode (Raw, Trim_Nul => False);
        Trimmed : Ada_Text := Decode (Encoded);
        Single_Nul : C_Text (Top .. Top) := (others => Nul_Value);
        Only_Nul : C_Text := Encode (Empty_Ada);
        Null_Result : Ada_Text := Decode (Single_Nul);
        Null_Untrimmed : Ada_Text := Decode (Empty_C, Trim_Nul => False);
        Unterminated : C_Text (Top - 1 .. Top) := (others => To_C (Symbol));
        Untrimmed : Ada_Text := Decode (Unterminated, Trim_Nul => False);
        Buffer : C_Text (Top - 3 .. Top) := (others => Sentinel);
        Exact : C_Text (Top - 1 .. Top) := (others => Sentinel);
        Ada_Buffer : Ada_Text (17 .. 20) := (others => Ada_Sentinel);
        Ada_Last : Ada_Text (Integer'Last .. Integer'Last) := (others => Ada_Sentinel);
        type Element_Ref is access all C_Char;
        View : Element_Ref := Buffer (Top - 2)'Access;
        Exact_View : Element_Ref := Exact (Top - 1)'Access;
        C_Count : C.size_t;
        Ada_Count : Natural;
    begin
        if To_Ada (To_C (Symbol)) /= Symbol
            or else C_Char'Pos (To_C (Symbol)) /= Ada_Char'Pos (Symbol)
            or else Ada_Char'Pos (To_Ada (Nul_Value)) /= 0
            or else Encoded'First /= 0 or else Encoded'Last /= 3
            or else Encoded (0) /= To_C (Symbol) or else Encoded (3) /= Nul_Value
            or else not Contains_Nul (Encoded) or else not Contains_Nul (Single_Nul)
            or else Contains_Nul (Empty_C) or else Contains_Nul (Unterminated)
            or else Raw'First /= 0 or else Raw'Last /= 2
            or else Back'First /= 1 or else Back /= Source
            or else Trimmed'First /= 1 or else Trimmed'Last /= 1 or else Trimmed (1) /= Symbol
            or else Only_Nul'First /= 0 or else Only_Nul'Last /= 0 or else Only_Nul (0) /= Nul_Value
            or else Null_Result'First /= 1 or else Null_Result'Last /= 0
            or else Null_Untrimmed'First /= 1 or else Null_Untrimmed'Last /= 0
            or else Untrimmed'First /= 1 or else Untrimmed /= Ada_Text'(Symbol, Symbol)
        then
            raise Program_Error;
        end if;

        Encode (At_Last, Buffer (Top - 2 .. Top), C_Count);
        if C_Count /= 2 or else View.all /= To_C (Symbol) or else Buffer (Top - 1) /= Nul_Value
            or else Buffer (Top - 3) /= Sentinel or else Buffer (Top) /= Sentinel
        then
            raise Program_Error;
        end if;
        View.all := Sentinel;
        if Buffer (Top - 2) /= Sentinel then
            raise Program_Error;
        end if;
        Exact := Encode (At_Last);
        if Exact'First /= Top - 1 or else Exact_View.all /= To_C (Symbol)
            or else Exact (Top) /= Nul_Value or else Decode (Exact) /= At_Last
        then
            raise Program_Error;
        end if;

        Decode (Exact, Ada_Last, Ada_Count);
        if Ada_Count /= 1 or else Ada_Last (Integer'Last) /= Symbol then
            raise Program_Error;
        end if;
        Decode (Raw, Ada_Buffer, Ada_Count, Trim_Nul => False);
        if Ada_Count /= 3 or else Ada_Buffer (17 .. 19) /= Source or else Ada_Buffer (20) /= Ada_Sentinel then
            raise Program_Error;
        end if;
        Encode (Source, Buffer, C_Count, Append_Nul => False);
        if C_Count /= 3 or else Buffer (Top) /= Sentinel or else Decode (Buffer) /= Trimmed then
            raise Program_Error;
        end if;

        Encode (Empty_Ada, Empty_C, C_Count, Append_Nul => False);
        Decode (Empty_C, Empty_Ada, Ada_Count, Trim_Nul => False);
        if C_Count /= 0 or else Ada_Count /= 0 then
            raise Program_Error;
        end if;
        Decode (Single_Nul, Empty_Ada, Ada_Count);
        if Ada_Count /= 0 then
            raise Program_Error;
        end if;
        Encode (Empty_Ada, Buffer (Top .. Top), C_Count);
        if C_Count /= 1 or else Buffer (Top) /= Nul_Value then
            raise Program_Error;
        end if;
        Buffer (Top) := Sentinel;

        begin
            declare
                Invalid : C_Text := Encode (Empty_Ada, Append_Nul => False);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        C_Count := 99;
        begin
            Encode (Empty_Ada, Empty_C, C_Count);
            raise Program_Error;
        exception
            when Constraint_Error =>
                if C_Count /= 99 then
                    raise Program_Error;
                end if;
        end;
        begin
            Encode (Source, Buffer (Top .. Top), C_Count);
            raise Program_Error;
        exception
            when Constraint_Error =>
                if C_Count /= 99 or else Buffer (Top) /= Sentinel then
                    raise Program_Error;
                end if;
        end;
        Ada_Count := 99;
        begin
            Decode (Encoded, Empty_Ada, Ada_Count);
            raise Program_Error;
        exception
            when Constraint_Error =>
                if Ada_Count /= 99 then
                    raise Program_Error;
                end if;
        end;
        begin
            Decode (Raw, Ada_Last, Ada_Count, Trim_Nul => False);
            raise Program_Error;
        exception
            when Constraint_Error =>
                if Ada_Count /= 99 or else Ada_Last (Integer'Last) /= Symbol then
                    raise Program_Error;
                end if;
        end;
        begin
            declare
                Invalid : Ada_Text := Decode (Unterminated);
            begin
                raise Program_Error;
            end;
        exception
            when C.Terminator_Error => null;
        end;
        begin
            Decode (Empty_C, Ada_Buffer, Ada_Count);
            raise Program_Error;
        exception
            when C.Terminator_Error =>
                if Ada_Count /= 99 then
                    raise Program_Error;
                end if;
        end;
    end Check_Family;

    procedure Check_Char is new Check_Family
        (Character, C.char, String, C.char_array, Character'Val (16#FF#), C.nul,
         C.To_C, C.To_Ada, C.Is_Nul_Terminated, C.To_C, C.To_Ada, C.To_C, C.To_Ada);
    procedure Check_Wchar is new Check_Family
        (Wide_Character, C.wchar_t, Wide_String, C.wchar_array, Wide_Character'Val (16#FFFF#), C.wide_nul,
         C.To_C, C.To_Ada, C.Is_Nul_Terminated, C.To_C, C.To_Ada, C.To_C, C.To_Ada);
    procedure Check_Char16 is new Check_Family
        (Wide_Character, C.char16_t, Wide_String, C.char16_array, Wide_Character'Val (16#D800#), C.char16_nul,
         C.To_C, C.To_Ada, C.Is_Nul_Terminated, C.To_C, C.To_Ada, C.To_C, C.To_Ada);
    procedure Check_Char32 is new Check_Family
        (Wide_Wide_Character, C.char32_t, Wide_Wide_String, C.char32_array,
         Wide_Wide_Character'Val (16#7FFF_FFFF#), C.char32_nul,
         C.To_C, C.To_Ada, C.Is_Nul_Terminated, C.To_C, C.To_Ada, C.To_C, C.To_Ada);

    procedure Check_Wchar_Mapping is
        Beyond_Wide : Integer := 16#10000#;
    begin
        if C.wchar_t'Size = 32 then
            declare
                Invalid : C.wchar_t := C.wchar_t'Val (Beyond_Wide);
                Text : C.wchar_array (5 .. 7) := (C.To_C (Wide_Character'('A')), Invalid, C.wide_nul);
                Target : Wide_String (1 .. 2);
                Count : Natural := 99;
                Code : Wide_Character;
            begin
                begin
                    Code := C.To_Ada (Invalid);
                    raise Program_Error;
                exception
                    when Constraint_Error => null;
                end;
                for Attempt in 1 .. 4 loop
                    begin
                        declare
                            Result : Wide_String := C.To_Ada (Text);
                        begin
                            raise Program_Error;
                        end;
                    exception
                        when Constraint_Error => null;
                    end;
                end loop;
                begin
                    C.To_Ada (Text, Target, Count);
                    raise Program_Error;
                exception
                    when Constraint_Error =>
                        if Count /= 99 then
                            raise Program_Error;
                        end if;
                end;
                Text (6) := C.wide_nul;
                Text (7) := Invalid;
                if C.To_Ada (Text) /= Wide_String'("A") then
                    raise Program_Error;
                end if;
            end;
        end if;
    end Check_Wchar_Mapping;
begin
    if C.char_array'Component_Size /= C.CHAR_BIT
        or else C.wchar_array'Component_Size /= C.wchar_t'Size
        or else C.char16_array'Component_Size /= 16 or else C.char32_array'Component_Size /= 32
    then
        raise Program_Error;
    end if;
    Check_Char;
    Check_Wchar;
    Check_Char16;
    Check_Char32;
    Check_Wchar_Mapping;
    Ada.Text_IO.Put_Line ("Interfaces.C character conversions passed");
end InterfacesCharacters;
