package body Rename_Library is
    function Original (X : Integer) return Integer is
    begin
        return X * 2;
    end Original;
    function Completed (Value : Integer) return Integer renames Original;
end Rename_Library;
