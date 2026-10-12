package Bundled_A is
    Value : Integer := 10;
end Bundled_A;

with Bundled_B;
package body Bundled_A is
begin
    Value := Bundled_B.Value + 1;
end Bundled_A;

with Bundled_A;
package Bundled_B is
    Value : Integer := Bundled_A.Value + 1;
end Bundled_B;

with Ada.Text_IO;
with Bundled_A;
procedure Bundle_Main is
begin
    Ada.Text_IO.Put_Line (Integer'Image (Bundled_A.Value));
end Bundle_Main;
