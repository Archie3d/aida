with Ada.Text_IO;
with Ada.Strings.Wide_Maps; use Ada.Strings.Wide_Maps;
with Ada.Strings.Wide_Wide_Maps; use Ada.Strings.Wide_Wide_Maps;
with Ada.Strings.Wide_Bounded;
with Ada.Strings.Wide_Wide_Bounded;
with WideAttributeViews; use WideAttributeViews;
procedure WideOverloads is
    Set : Wide_Character_Set := To_Set ("ΩЖ");
    Full_Set : Wide_Wide_Character_Set := To_Set ("Ω😀");
    Map : Wide_Character_Mapping := To_Mapping ("Ω", "Ж");
    Full_Map : Wide_Wide_Character_Mapping := To_Mapping ("😀", "Ω");
    package B is new Ada.Strings.Wide_Bounded.Generic_Bounded_Length (8);
    package Full_B is new Ada.Strings.Wide_Wide_Bounded.Generic_Bounded_Length (8);
    use B;
    use Full_B;
    Bounded : Bounded_Wide_String := To_Bounded_Wide_String ("ΩЖ");
    Full_Bounded : Bounded_Wide_Wide_String := To_Bounded_Wide_Wide_String ("Ω😀");
    Text : Wide_String := To_Wide_String (Bounded);
    Full_Text : Wide_Wide_String := To_Wide_Wide_String (Full_Bounded);
    generic
        with function Image (Arg : Integer) return Wide_String is <>;
    function Boxed return Wide_String;
    function Boxed return Wide_String is
    begin
        return Image (4);
    end Boxed;
    function From_Box is new Boxed;
    function Complete (Arg : Integer) return Wide_Wide_String;
    function Complete (Arg : Integer) return Wide_Wide_String renames Integer'Wide_Wide_Image;
begin
    if not Is_In ('Ω', Set) or not Is_In ('😀', Full_Set)
        or Value (Map, 'Ω') /= 'Ж' or Value (Full_Map, '😀') /= 'Ω'
        or Text /= "ΩЖ" or Full_Text /= "Ω😀"
        or From_Box /= " 4" or Complete (4) /= " 4" then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("wide overloads passed");
end WideOverloads;
