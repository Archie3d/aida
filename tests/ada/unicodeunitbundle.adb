package Α is
    Value : constant Integer := 17;
end Α;
package Β is
    Value : constant Integer := 25;
end Β;
with Α;
with Β;
with Ada.Text_IO;
procedure Unicodeunitbundle is
    𐐀 : Integer := Α.Value;
begin
    𐐨 := 𐐨 + Β.Value;
    Ada.Text_IO.Put_Line (Integer'Image (𐐀));
end Unicodeunitbundle;
