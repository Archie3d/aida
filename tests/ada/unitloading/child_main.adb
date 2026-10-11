with Load_Parent.Child;
with Ada.Text_IO;
procedure Child_Main is
begin
    Ada.Text_IO.Put_Line (Integer'Image (Load_Parent.Child.Result));
end Child_Main;
