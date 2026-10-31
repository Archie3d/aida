with Ada.Text_IO;
with Ada.Characters.Conversions;
with Ada.Characters.Handling;
with Ada.Exceptions;
with Ada.Tags;
procedure Widefoundation is
    use Ada.Characters.Conversions;
    Πλάτων : constant Integer := 17;
    Straße : constant Integer := 23;
    W : Wide_String := "éΩ";
    WW : Wide_Wide_String := "éΩ😀";
    Narrow : String := "é";
    type Λέξη is (αβ, Größe);
    type Glyph is ('Ω', '😀');
    Failure_Ω : exception;
    Check_Number : Integer := 0;
    procedure Check (Value : Boolean) is
    begin
        Check_Number := Check_Number + 1;
        if not Value then raise Program_Error with Integer'Image (Check_Number); end if;
    end Check;
    type Τύπος is tagged null record;
    function Decode_Name (Name : String) return Wide_Wide_String;
    pragma Import (C, Decode_Name, "__ada_utf8_wide_wide");
    procedure Σ (Π : Integer := 23) is
    begin
        Check (ΠΛΆΤΩΝ + STRASSE = 40 and Π = 23);
    end Σ;
    subtype Greek is Wide_Character range 'Α' .. 'Ω';
    X : Wide_Wide_Character := '😀';
begin
    σ;
    for Βήμα in 1 .. 2 loop
        ς;
    end loop;
    Check (Decode_Name (Ada.Tags.Expanded_Name (Τύπος'Tag)) = "WIDEFOUNDATION.ΤΎΠΟΣ");
    Check (W'Length = 2 and WW'Length = 3 and Narrow'Length = 1);
    Check (Character'Pos (Narrow (1)) = 233);
    Check (Wide_Character'Pos (W (2)) = 937);
    Check (Wide_Wide_Character'Pos (WW (3)) = 128512);
    Check (Wide_Character'Size = 16 and Wide_Wide_Character'Size = 32);
    Check (To_String (W, '?') = "é?");
    Check (To_Wide_String (WW, '!') = "éΩ!");
    Check (To_Wide_Wide_String (W) = "éΩ");
    Check (To_Wide_Wide_String (Narrow) = "é");
    Check (not Is_Character (W (2)) and not Is_String (W));
    Check (not Is_Wide_Character (X) and not Is_Wide_String (WW));
    Check (Is_Character (Wide_Character'('é')));
    Check (Ada.Characters.Handling.To_Wide_String (Narrow) = "é");
    Check (Wide_Character'Wide_Image ('Ω') = "'Ω'");
    Check (Wide_Wide_Character'Wide_Wide_Image (X) = "'😀'");
    Check (Wide_Wide_Character'Wide_Wide_Value (Wide_Wide_Character'Wide_Wide_Image (X)) = X);
    Check (Wide_Character'Image ('Ω') = "Hex_000003A9");
    Check (Wide_Character'Value ("Hex_000003A9") = 'Ω');
    Check (Wide_Character'Wide_Value ("'Ω'") = 'Ω');
    Check (Integer'Wide_Value ("123") = 123);
    Check (Integer'Wide_Image (123) = " 123");
    Check (Float'Wide_Wide_Value ("1.25") = 1.25);
    Check (Character'Image (Character'Val (0)) = "NUL");
    Check (Character'Value ("nul") = Character'Val (0));
    Check (Character'Width = 12 and Wide_Character'Wide_Width = 12);
    Check (Greek'Wide_Width = 3 and Greek'Width = 12);
    Check (ΛΈΞΗ'Wide_Wide_Value ("αβ") = αβ);
    Check (Λέξη'Wide_Image (αβ) = "ΑΒ");
    Check (Glyph'Wide_Wide_Image ('😀') = "'😀'");
    Check (Glyph'Wide_Width = 14 and Glyph'Wide_Wide_Width = 3);
    Check (Ada.Exceptions.Wide_Exception_Name (Failure_Ω'Identity) = "WIDEFOUNDATION.FAILURE_Ω");
    Check (Ada.Exceptions.Wide_Wide_Exception_Name (Failure_Ω'Identity) = "WIDEFOUNDATION.FAILURE_Ω");
    declare
        Shifted : Wide_Wide_String (7 .. 9) := WW;
        Empty : Wide_String (2 .. 1);
    begin
        Check (To_String (Shifted, '?') = "é??");
        Check (To_Wide_String (Shifted)'First = 1);
        Check (To_Wide_String (Shifted)'Length = 3);
        Check (Is_String (Empty) and To_String (Empty)'Length = 0);
    end;
    declare
        Edge : Wide_String (1 .. 1) := (1 => Wide_Character'Val (65535));
        Big : Wide_Wide_String := To_Wide_Wide_String (Edge);
        Maximum : Wide_Wide_Character := Wide_Wide_Character'Val (2147483647);
    begin
        Check (Wide_Wide_Character'Pos (Big (1)) = 65535);
        Check (Wide_Wide_Character'Wide_Wide_Value (Wide_Wide_Character'Wide_Wide_Image (Maximum)) = Maximum);
    end;
    Ada.Text_IO.Put_Line ("wide foundation passed");
end Widefoundation;
