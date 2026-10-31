with Rename_Library; use Rename_Library;
procedure Renamelibrary is
    function Next (Value : Integer) return Integer renames Integer'Succ;
    function Read (Text : String) return Integer renames Integer'Value;
    A : Callback := Alias'Access;
begin
    Print (Image (Alias));
    Print (Image (Completed));
    F := null;
    Print (Image (Saved (5)));
    Print (Image (Next (Read ("6"))));
    if A = Original'Access then
        Print ("library identity");
    end if;
end Renamelibrary;
