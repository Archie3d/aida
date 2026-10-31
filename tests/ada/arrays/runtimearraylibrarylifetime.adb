with Ada.Text_IO;
with Runtime_Array_Lifetime;
procedure RuntimeArrayLibraryLifetime is
begin
    Runtime_Array_Lifetime.Check;
    Ada.Text_IO.Put_Line ("library arrays alive after elaboration");
end RuntimeArrayLibraryLifetime;
