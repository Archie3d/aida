package body Ada.Tags is
    function Interface_Count (T : Tag) return Natural;
    pragma Import (C, Interface_Count, "__ada_tag_interface_count");
    function Interface_At (T : Tag; Position : Positive) return Tag;
    pragma Import (C, Interface_At, "__ada_tag_interface_at");

    function Interface_Ancestor_Tags (T : Tag) return Tag_Array is
        Result : Tag_Array (1 .. Interface_Count (T));
    begin
        for Index in Result'Range loop
            Result (Index) := Interface_At (T, Index);
        end loop;
        return Result;
    end Interface_Ancestor_Tags;
end Ada.Tags;
