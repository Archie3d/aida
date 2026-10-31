package Runtime_Array_Overflow is
    Last : Long_Integer := Long_Integer'Last;
    type Vector is array (Long_Integer range 0 .. Last) of Integer;
end Runtime_Array_Overflow;
with Runtime_Array_Overflow;
procedure RuntimeArrayLibraryOverflow is
begin
    raise Program_Error;
end RuntimeArrayLibraryOverflow;
