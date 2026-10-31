with Ada.Text_IO;
with Ada.Wide_Text_IO;
with Ada.Integer_Wide_Text_IO;
with Ada.Long_Integer_Wide_Text_IO;
with Ada.Float_Wide_Text_IO;
with Ada.Long_Float_Wide_Text_IO;
procedure Wideiolibrary is
    package W renames Ada.Wide_Text_IO;
    use type W.Count;
    use type W.File_Mode;
    type Modulus is mod 256;
    type Fixed is delta 0.125 range -100.0 .. 100.0;
    type Decimal is delta 0.01 digits 6;
    type Choice is (Αλφα, Βητα);
    type Glyph is ('Ω', 'Ж');
    package M is new W.Modular_IO (Modulus);
    package Fx is new W.Fixed_IO (Fixed);
    package D is new W.Decimal_IO (Decimal);
    package E is new W.Enumeration_IO (Choice);
    package G is new W.Enumeration_IO (Glyph);
    F : W.File_Type;
    S : Wide_String (5 .. 12);
    Last : Natural;
    I : Integer;
    L : Long_Integer;
    R : Float;
    LR : Long_Float;
    Modular_Value : Modulus;
    Fixed_Value : Fixed;
    Decimal_Value : Decimal;
    Enum_Value : Choice;
    Character_Value : Wide_Character;
    Flag : Boolean;
    Check_Number : Natural := 0;
    procedure Check (Condition : Boolean) is
    begin
        Check_Number := Check_Number + 1;
        if not Condition then
            raise Program_Error with Integer'Image (Check_Number);
        end if;
    end Check;
    function Selected return W.File_Type is
    begin
        return W.Current_Output;
    end Selected;
begin
    Check (not W.Is_Open (F));
    begin
        W.Close (F);
        raise Program_Error;
    exception
        when W.Status_Error => null;
    end;
    W.Create (F);
    Check (W.Is_Open (F) and W.Mode (F) = W.Out_File);
    W.Set_Output (F);
    declare
        Borrowed : constant W.File_Type := Selected;
    begin
        W.Put (Borrowed, "ΩéЖ");
    end;
    Check (W.Is_Open (F) and W.Col = 4);
    W.New_Line;
    Ada.Integer_Wide_Text_IO.Put (42, Width => 0);
    W.Put (' ');
    Ada.Long_Integer_Wide_Text_IO.Put (-9000000000, Width => 0);
    W.Put (' ');
    Ada.Float_Wide_Text_IO.Put (1.25, Fore => 1, Aft => 2, Exp => 0);
    W.Put (' ');
    Ada.Long_Float_Wide_Text_IO.Put (2.5, Fore => 1, Aft => 1, Exp => 0);
    W.Put (' ');
    M.Put (255, Width => 0, Base => 16);
    W.Put (' ');
    Fx.Put (1.5, Fore => 1, Aft => 3, Exp => 0);
    W.Put (' ');
    D.Put (-2.25, Fore => 1, Aft => 2, Exp => 0);
    W.Put (' ');
    E.Put (Αλφα, Set => W.Lower_Case);
    W.Put (' ');
    G.Put ('Ж');
    W.New_Line;
    W.Flush;
    Check (W.Line = 3);
    begin
        W.Reset (F, W.In_File);
        raise Program_Error;
    exception
        when W.Mode_Error => null;
    end;
    W.Set_Output (W.Standard_Output);
    W.Reset (F, W.In_File);
    W.Set_Input (F);
    W.Look_Ahead (Character_Value, Flag);
    Check (not Flag and Character_Value = 'Ω' and W.Col (F) = 1);
    W.Get_Line (S, Last);
    Check (Last = 7 and S (5 .. Last) = "ΩéЖ");
    Ada.Integer_Wide_Text_IO.Get (I);
    Ada.Long_Integer_Wide_Text_IO.Get (L);
    Ada.Float_Wide_Text_IO.Get (R);
    Ada.Long_Float_Wide_Text_IO.Get (LR);
    M.Get (Modular_Value);
    Fx.Get (Fixed_Value);
    D.Get (Decimal_Value);
    E.Get (Enum_Value);
    declare
        Glyph_Value : Glyph;
    begin
        G.Get (Glyph_Value);
        Check (Glyph_Value = 'Ж');
    end;
    Check (I = 42 and L = -9000000000 and R = 1.25 and LR = 2.5);
    Check (Modular_Value = 255 and Fixed_Value = 1.5 and Decimal_Value = -2.25);
    Check (Enum_Value = Αλφα and W.End_Of_Line and W.End_Of_File);
    W.Close (F);
    -- String numeric profiles retain caller bounds and native cell widths.
    Ada.Integer_Wide_Text_IO.Put (S, -42);
    Ada.Integer_Wide_Text_IO.Get (S, I, Last);
    Check (I = -42 and Last = S'Last);
    M.Put (S, 255, Base => 16);
    M.Get (S, Modular_Value, Last);
    Check (Modular_Value = 255 and Last = S'Last);
    Ada.Float_Wide_Text_IO.Put (S, 1.25, Aft => 2, Exp => 0);
    Ada.Float_Wide_Text_IO.Get (S, R, Last);
    Check (R = 1.25 and Last = S'Last);
    Fx.Put (S, -1.5, Aft => 3, Exp => 0);
    Fx.Get (S, Fixed_Value, Last);
    Check (Fixed_Value = -1.5 and Last = S'Last);
    D.Put (S, 3.25, Aft => 2, Exp => 0);
    D.Get (S, Decimal_Value, Last);
    Check (Decimal_Value = 3.25 and Last = S'Last);
    E.Put (S, Βητα, Set => W.Lower_Case);
    Check (S (5 .. 8) = "βητα");
    E.Get (S, Enum_Value, Last);
    Check (Enum_Value = Βητα and Last = 8);
    begin
        Ada.Integer_Wide_Text_IO.Get ("Ω", I, Last);
        raise Program_Error;
    exception
        when W.Data_Error => null;
    end;
    -- Wide numeric fields cannot alias a digit by truncating its code point.
    W.Create (F);
    W.Put_Line (F, "İ");
    W.Put_Line (F, "İ");
    W.Reset (F, W.In_File);
    begin
        M.Get (F, Modular_Value, Width => 1);
        raise Program_Error;
    exception
        when W.Data_Error => null;
    end;
    W.Skip_Line (F);
    begin
        Fx.Get (F, Fixed_Value, Width => 1);
        raise Program_Error;
    exception
        when W.Data_Error => null;
    end;
    W.Close (F);
    -- Lengths count characters, and a full line buffer leaves its terminator.
    W.Create (F);
    W.Set_Line_Length (F, 2);
    W.Put (F, "ΩéЖ");
    Check (W.Line (F) = 2 and W.Col (F) = 2);
    W.New_Line (F);
    W.New_Page (F);
    Check (W.Page (F) = 2);
    W.Reset (F, W.In_File);
    declare
        Two : Wide_String (3 .. 4);
        Empty : Wide_String (4 .. 3);
    begin
        W.Get_Line (F, Empty, Last);
        Check (Last = 3);
        W.Get_Line (F, Two, Last);
        Check (Two = "Ωé" and Last = 4 and W.End_Of_Line (F));
        W.Get_Line (F, Two, Last);
        Check (Last = 2);
        W.Get_Line (F, S, Last);
        Check (Last = 5 and S (5) = 'Ж');
    end;
    W.Close (F);
    -- Owned string line results preserve NUL and support repeated consumption.
    W.Create (F);
    W.Put_Line (F, "Ω" & Wide_Character'Val (0) & 'Ж');
    W.Put_Line (F, "");
    W.Reset (F, W.In_File);
    declare
        Line : constant Wide_String := W.Get_Line (F);
    begin
        Check (Line'First = 1 and Line'Length = 3);
        Check (Line (1) = 'Ω' and Line (2) = Wide_Character'Val (0) and Line (3) = 'Ж');
    end;
    Check (W.Get_Line (F)'Length = 0);
    W.Close (F);
    -- Unopened, closed and borrowed file finalizers must not consume pool slots.
    for Index in 1 .. 40 loop
        declare
            Local : W.File_Type;
        begin
            W.Create (Local);
            W.Put (Local, 'Ω');
        end;
    end loop;
    -- A borrowed result may become an owning variable after close/reopen.
    for Index in 1 .. 40 loop
        W.Create (F);
        W.Set_Output (F);
        declare
            Local : W.File_Type := Selected;
        begin
            W.Close (Local);
            W.Create (Local);
            W.Put (Local, 'Ω');
        end;
    end loop;
    Ada.Text_IO.Put_Line ("Wideiolibrary passed");
end Wideiolibrary;
