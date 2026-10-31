package body Access_Result_Base is
    function Sum (Self : Root; Other : access Root := Make) return Integer is
    begin
        return Read (Root'Class (Self)) + Read (Root'Class (Other.all));
    end Sum;
end Access_Result_Base;
