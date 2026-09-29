package body Ada.Tags is
    function Interface_Ancestor_Tags (T : Tag) return Tag_Array is
    begin
        if T = No_Tag then
            raise Tag_Error;
        end if;
        -- The supported tagged subset has single inheritance, without interfaces.
        return (1 .. 0 => No_Tag);
    end Interface_Ancestor_Tags;
end Ada.Tags;
