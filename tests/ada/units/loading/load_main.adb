with Load_A;
with Load_B;
with Ada.Text_IO;
procedure Load_Main is
begin
    Ada.Text_IO.Put_Line (Integer'Image (Load_B.Read_Value));
end Load_Main;
