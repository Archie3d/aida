package body Packed_Signed_Model is
    function Copy (item : Values) return Values is
    begin
        return item;
    end Copy;
    procedure Negate (item : in out Values) is
    begin
        for i in item'Range loop
            item (i) := -item (i);
        end loop;
    end Negate;
end Packed_Signed_Model;
