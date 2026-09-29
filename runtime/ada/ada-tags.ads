package Ada.Tags is
    type Tag is private;
    No_Tag : constant Tag;
    Tag_Error : exception;

    function Expanded_Name (T : Tag) return String;
    pragma Import (C, Expanded_Name, "__ada_tag_name");
    function External_Tag (T : Tag) return String;
    pragma Import (C, External_Tag, "__ada_tag_name");
    function Internal_Tag (External : String) return Tag;
    pragma Import (C, Internal_Tag, "__ada_tag_internal");
    function Descendant_Tag (External : String; Ancestor : Tag) return Tag;
    pragma Import (C, Descendant_Tag, "__ada_tag_descendant");
    function Is_Descendant_At_Same_Level (Descendant, Ancestor : Tag) return Boolean;
    pragma Import (C, Is_Descendant_At_Same_Level, "__ada_tag_is_descendant");
    function Parent_Tag (T : Tag) return Tag;
    pragma Import (C, Parent_Tag, "__ada_tag_parent");
    type Tag_Array is array (Positive range <>) of Tag;
    function Interface_Ancestor_Tags (T : Tag) return Tag_Array;
    function Is_Abstract (T : Tag) return Boolean;
    pragma Import (C, Is_Abstract, "__ada_tag_is_abstract");
private
    type Tag is access Integer;
    No_Tag : constant Tag := null;
end Ada.Tags;
