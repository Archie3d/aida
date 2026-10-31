with widewideStringCategories;
with Ada.Strings; use Ada.Strings;
with Ada.Strings.Wide_Wide_Maps; use Ada.Strings.Wide_Wide_Maps;
with Ada.Strings.Wide_Wide_Maps.Wide_Wide_Constants;
with Ada.Strings.Wide_Wide_Fixed;
with Ada.Strings.Wide_Wide_Bounded;
with Ada.Strings.Wide_Wide_Unbounded;
with Ada.Wide_Wide_Text_IO;
with Ada.Wide_Wide_Text_IO.Wide_Wide_Bounded_IO;
with Ada.Wide_Wide_Text_IO.Wide_Wide_Unbounded_IO;
with Ada.Streams.Stream_IO;
procedure WideWideStringLimits is
    package C renames Ada.Strings.Wide_Wide_Maps.Wide_Wide_Constants;
    package F renames Ada.Strings.Wide_Wide_Fixed;
    package B is new Ada.Strings.Wide_Wide_Bounded.Generic_Bounded_Length (8);
    package U renames Ada.Strings.Wide_Wide_Unbounded;
    package BI is new Ada.Wide_Wide_Text_IO.Wide_Wide_Bounded_IO (B);
    package UI renames Ada.Wide_Wide_Text_IO.Wide_Wide_Unbounded_IO;
    package IO renames Ada.Wide_Wide_Text_IO;
    use type B.Bounded_Wide_Wide_String;
    use type U.Unbounded_Wide_Wide_String;
    Last : constant Wide_Wide_Character := Wide_Wide_Character'Last;
    First : constant Wide_Wide_Character := Wide_Wide_Character'First;
    Top : Wide_Wide_Character_Set := To_Set (Wide_Wide_Character_Range'(Wide_Wide_Character'Pred (Last), Last));
    Bottom : Wide_Wide_Character_Set := To_Set (First);
    Saved : Wide_Wide_Character_Set := Top;
    All_Values : Wide_Wide_Character_Set := not Null_Set;
    Map : Wide_Wide_Character_Mapping := To_Mapping (Wide_Wide_String'(Last, '😀', First), Wide_Wide_String'(First, 'Ж', Last));
    Copy : Wide_Wide_Character_Mapping := Map;
    Text : Wide_Wide_String (Positive'Last - 2 .. Positive'Last) := "😀Ж😀";
    Bounded : B.Bounded_Wide_Wide_String := B.To_Bounded_Wide_Wide_String (Text);
    Unbounded : U.Unbounded_Wide_Wide_String := U.To_Unbounded_Wide_Wide_String (Text);
    File : IO.File_Type;
    Stream_File : Ada.Streams.Stream_IO.File_Type;
    procedure Check (Ok : Boolean) is
    begin
        if not Ok then
            raise Program_Error;
        end if;
    end Check;
begin
    begin
        declare
            Too_Large : Wide_Wide_String := To_Sequence (All_Values);
        begin
            raise Program_Error;
        end;
    exception
        when Storage_Error => null;
    end;
    Check (To_Ranges (All_Values)'Length = 1);
    Check (Is_In (First, All_Values) and Is_In (Last, All_Values));
    Check ((Top and Bottom) = Null_Set and (Top or not Top) = All_Values);
    Check (not (Top xor All_Values) = Top);
    Check (Top - All_Values = Null_Set);
    Check (To_Sequence (Top) = Wide_Wide_String'(Wide_Wide_Character'Pred (Last), Last));
    Top := Top or Bottom;
    Check (Saved /= Top and To_Ranges (Top)'Length = 2);
    Check (not Is_In ('😀', C.Letter_Set) and Value (C.Lower_Case_Map, '😀') = '😀');
    Check (Is_In (Wide_Wide_Character'Val (255), C.Character_Set));
    Check (not Is_In (Wide_Wide_Character'Val (256), C.Character_Set));
    Check (Is_In (Wide_Wide_Character'Val (65535), C.Wide_Character_Set));
    Check (not Is_In (Wide_Wide_Character'Val (65536), C.Wide_Character_Set));
    Check (Value (Map, First) = Last and Value (Map, Last) = First);
    Map := Identity;
    Check (Value (Copy, '😀') = 'Ж' and Value (Map, Last) = Last);
    Check (To_Domain (Copy) = Wide_Wide_String'(First, '😀', Last));
    Check (To_Range (Copy) = Wide_Wide_String'(Last, 'Ж', First));
    Check (F.Index (Text, "Ж") = Positive'Last - 1);
    Check (F.Translate (Text, Copy) = "ЖЖЖ");
    Check (B.Translate (Bounded, Copy) = "ЖЖЖ");
    Check (U.Translate (Unbounded, Copy) = "ЖЖЖ");
    IO.Create (File);
    BI.Put_Line (File, Bounded);
    UI.Put_Line (File, Unbounded);
    IO.Reset (File, IO.In_File);
    Check (BI.Get_Line (File) = "😀Ж😀" and UI.Get_Line (File) = "😀Ж😀");
    IO.Close (File);
    Ada.Streams.Stream_IO.Create (Stream_File);
    U.Unbounded_Wide_Wide_String'Output (Ada.Streams.Stream_IO.Stream (Stream_File), Unbounded);
    Ada.Streams.Stream_IO.Reset (Stream_File, Ada.Streams.Stream_IO.In_File);
    Unbounded := U.Null_Unbounded_Wide_Wide_String;
    Unbounded := U.Unbounded_Wide_Wide_String'Input (Ada.Streams.Stream_IO.Stream (Stream_File));
    Check (Unbounded = "😀Ж😀");
    Ada.Streams.Stream_IO.Close (Stream_File);
    begin
        Map := To_Mapping (Wide_Wide_String'(Last, Last), Wide_Wide_String'(Last, First));
        raise Program_Error;
    exception
        when Translation_Error => null;
    end;
    IO.Put_Line ("wide string limits passed");
end WideWideStringLimits;
