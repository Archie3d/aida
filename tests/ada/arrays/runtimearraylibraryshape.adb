package Runtime_Array_Bad_Shape is
    N : Integer := 2;
    subtype Text is String (1 .. N);
    generic
        X : Text;
    package Copy is
    end Copy;
    package Wrong is new Copy ("abc");
end Runtime_Array_Bad_Shape;
with Runtime_Array_Bad_Shape;
procedure RuntimeArrayLibraryShape is
begin
    raise Program_Error;
end RuntimeArrayLibraryShape;
