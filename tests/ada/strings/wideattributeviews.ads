package WideAttributeViews is
    subtype Small is Integer range 1 .. 3;
    function Image (Arg : Small) return Wide_String renames Small'Wide_Image;
    function Image (Arg : Small) return Wide_Wide_String renames Small'Wide_Wide_Image;
    function Value (Arg : Wide_String) return Small renames Small'Wide_Value;
    function Value (Arg : Wide_Wide_String) return Small renames Small'Wide_Wide_Value;
    function Base_Image (Arg : Integer) return Wide_String renames Small'Base'Wide_Image;
end WideAttributeViews;
