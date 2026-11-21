with Ada.Text_IO;
with Interfaces.C;

procedure InterfacesNative is
    package C renames Interfaces.C;
    use type C.char;
    use type C.wchar_t;
    use type C.char16_t;
    use type C.char32_t;
    use type C.int;
    use type C.short;
    use type C.unsigned;
    use type C.unsigned_short;
    use type C.size_t;

    procedure Fill (Text : in out C.char_array; Wide : in out C.wchar_array;
                   Units16 : out C.char16_array; Units32 : out C.char32_array;
                   Source : C.char_array; Count : C.size_t; Guard : C.int;
                   Tail : C.short; Last : C.unsigned_char; Scale : C.double;
                   Status : out C.unsigned_short);
    pragma Import (C, Fill, "aida_c_fill");
    type Fill_Procedure is access procedure
        (Text : in out C.char_array; Wide : in out C.wchar_array;
         Units16 : out C.char16_array; Units32 : out C.char32_array;
         Source : C.char_array; Count : C.size_t; Guard : C.int;
         Tail : C.short; Last : C.unsigned_char; Scale : C.double; Status : out C.unsigned_short);
    pragma Convention (C, Fill_Procedure);
    function Get_Fill return Fill_Procedure;
    pragma Import (C, Get_Fill, "aida_c_get_fill");

    type Words is array (C.size_t range <>) of C.unsigned;
    pragma Convention (C, Words);
    subtype Three_Words is Words (4 .. 6);
    function Update_Words (Value : in out Three_Words; Count : C.size_t) return C.unsigned;
    pragma Import (C, Update_Words, "aida_c_words");
    function Empty_Words (Value : in out Words; Count : C.size_t) return C.unsigned;
    pragma Import (C, Empty_Words, "aida_c_words");

    type Int_Ptr is access all C.int;
    pragma Convention (C, Int_Ptr);
    Number : aliased C.int := 123;
    subtype Valid_Ptr is not null Int_Ptr;
    function Pointer (Value : Int_Ptr; Expected : C.int) return Int_Ptr;
    pragma Import (C, Pointer, "aida_c_pointer");
    function Checked_Pointer (Value : Int_Ptr; Expected : C.int) return Valid_Ptr;
    pragma Import (C, Checked_Pointer, "aida_c_pointer");
    type Pointer_Function is access function (Value : Int_Ptr; Expected : C.int) return Int_Ptr;
    pragma Convention (C, Pointer_Function);
    procedure Replace (Value : in out Int_Ptr);
    pragma Import (C, Replace, "aida_c_replace_pointer");
    function Read_Char (Value : access constant C.char) return C.int;
    pragma Import (C, Read_Char, "aida_c_read_char");
    type Reader is access function (Value : access constant C.char) return C.int;
    pragma Convention (C, Reader);

    procedure Update (Value : in out C.int; Replacement : C.int);
    pragma Import (C, Update, "aida_c_update");
    type Code is (Start, Done);
    for Code use (Start => -20, Done => 241);
    for Code'Size use 32;
    procedure Set_Code (Value : in out Code);
    pragma Import (C, Set_Code, "aida_c_set_enum");

    procedure Stop (Text : out C.char_array; Value : C.unsigned_char);
    pragma Import (C, Stop, "aida_c_returns");
    pragma No_Return (Stop);
    type Stop_Procedure is access procedure (Text : out C.char_array; Value : C.unsigned_char);
    pragma Convention (C, Stop_Procedure);

    procedure Check_Arrays is
        Top : constant C.size_t := C.size_t'Last;
        Text : C.char_array (Top - 3 .. Top) := (others => C.char'('x'));
        Wide : C.wchar_array (17 .. 18) := (others => C.wchar_t'Val (16#1234#));
        Units16 : C.char16_array (23 .. 24);
        Units32 : C.char32_array (31 .. 32);
        type Char_Ptr is access all C.char;
        View : Char_Ptr := Text (Top - 1)'Access;
        Callback : Fill_Procedure := Get_Fill;
        Status : C.unsigned_short;
        Data : Words (101 .. 103) := (37, 16#8000_0000#, 0);
        Empty : Words (1 .. 0);
    begin
        if Callback /= Fill'Access then
            raise Program_Error;
        end if;
        for Attempt in 1 .. 2 loop
            Text (Top - 2) := C.char'('x');
            if Attempt = 1 then
                Fill (Text (Top - 2 .. Top), Wide, Units16, Units32,
                      C.To_C (String'("A" & Character'Val (0) & "Z"), Append_Nul => False),
                      3, 17009, -301, 237, 2.5, Status);
            else
                Callback (Text (Top - 2 .. Top), Wide, Units16, Units32,
                          (C.char'('A'), C.nul, C.char'('Z')),
                          3, 17009, -301, 237, 2.5, Status);
            end if;
            if Status /= 16#E205# or else Text (Top - 3) /= C.char'('x') or else Text (Top - 2) /= C.char'('Z')
                or else View.all /= C.char'Val (16#E9#) or else Text (Top) /= C.nul
                or else Wide (18) /= C.wchar_t'Val (16#FEDC#)
                or else Units16 (23) /= C.char16_t'Val (16#D800#)
                or else Units16 (24) /= C.char16_t'Val (16#F123#)
                or else Units32 (31) /= C.char32_t'Val (16#1F642#)
                or else Units32 (32) /= C.char32_t'Val (16#10FFFF#)
            then
                raise Program_Error;
            end if;
        end loop;
        if Update_Words (Data, 3) /= 37 or else Data (103) /= 16#F123_4567#
            or else Empty_Words (Empty, 0) /= 55
        then
            raise Program_Error;
        end if;
        begin
            declare
                Ignore : C.unsigned := Update_Words (Data (101 .. 102), 2);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
    end Check_Arrays;

    procedure Check_Pointers is
        Ref : Int_Ptr := Number'Access;
        Find : Pointer_Function := Pointer'Access;
        Read : Reader := Read_Char'Access;
        Text : C.char_array := C.To_C (String'("Q"));
        subtype Small is C.int range 0 .. 9;
        Digit : Small := 4;
        Letter : Code := Start;
        Guarded : Stop_Procedure := Stop'Access;
    begin
        if Pointer (Ref, 123) /= Ref or else Find (Ref, 124) /= null
            or else Read_Char (Text (0)'Access) /= 81 or else Read (Text (0)'Access) /= 81
            or else Read (null) /= -1
        then
            raise Program_Error;
        end if;
        begin
            Ref := Checked_Pointer (Ref, 124);
            raise Program_Error;
        exception
            when Constraint_Error =>
                if Ref /= Number'Access then
                    raise Program_Error;
                end if;
        end;
        Replace (Ref);
        if Ref.all /= 321 or else Number /= 123 then
            raise Program_Error;
        end if;
        Update (Digit, 6);
        begin
            Update (Digit, 10);
            raise Program_Error;
        exception
            when Constraint_Error =>
                if Digit /= 6 then
                    raise Program_Error;
                end if;
        end;
        Set_Code (Letter);
        if Letter /= Done then
            raise Program_Error;
        end if;
        begin
            Set_Code (Letter);
            raise Program_Error;
        exception
            when Constraint_Error =>
                if Letter /= Done then
                    raise Program_Error;
                end if;
        end;
        begin
            Stop (Text, 65);
            raise Constraint_Error;
        exception
            when Program_Error =>
                if Text (0) /= C.char'('A') then
                    raise Program_Error;
                end if;
        end;
        begin
            Guarded (Text, 66);
            raise Constraint_Error;
        exception
            when Program_Error =>
                if Text (0) /= C.char'('B') then
                    raise Program_Error;
                end if;
        end;
    end Check_Pointers;
begin
    Check_Arrays;
    Check_Pointers;
    Ada.Text_IO.Put_Line ("native C arrays and pointers passed");
end InterfacesNative;
