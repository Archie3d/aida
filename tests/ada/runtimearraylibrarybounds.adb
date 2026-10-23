package Runtime_Array_Bad_Bounds is
    N : Integer := 4;
    subtype Index is Integer range 1 .. 3;
    type Vector is array (Index range 1 .. N) of Integer;
end Runtime_Array_Bad_Bounds;
with Runtime_Array_Bad_Bounds;
procedure RuntimeArrayLibraryBounds is
begin
    raise Program_Error;
end RuntimeArrayLibraryBounds;
