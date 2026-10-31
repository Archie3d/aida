with Ada.Strings.Wide_Maps;
procedure WideProfileErrors is
    generic
        with function Encode (Arg : Integer) return Wide_String;
    package Images is
    end Images;
    package Narrow is new Images (Integer'Image);
    package Too_Wide is new Images (Integer'Wide_Wide_Image);
    package Width_Not_Subprogram is new Images (Integer'Wide_Width);
    function Wrong_Value (Arg : Wide_String) return Integer renames Integer'Wide_Wide_Value;
    function Wrong_Result (Arg : Integer) return String renames Integer'Wide_Image;
    function Character_Value (Arg : Wide_String) return Wide_Character renames Wide_Character'Value;
    function Object_Image (Arg : Integer) return Wide_String renames Integer'Wide_Width;
    function Image (Arg : Integer) return Wide_String renames Integer'Wide_Image;
    type Image_Access is access function (Arg : Integer) return Wide_String;
    Pointer : Image_Access := Image'Access;
    type Record_Type is record
        Item : Integer;
    end record;
    function Record_Image (Arg : Record_Type) return Wide_String renames Record_Type'Wide_Image;
begin
    null;
end WideProfileErrors;
