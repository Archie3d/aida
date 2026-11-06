package body Generic_Family.Child.Nested is
    function Parent_Calls return Integer is
    begin
        return Local_Count;
    end Parent_Calls;
end Generic_Family.Child.Nested;
