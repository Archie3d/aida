package body Impdef is
    function Equivalent_File_Names (Left, Right : String) return Boolean is
    begin
        return Left = Right;
    end Equivalent_File_Names;
end Impdef;
