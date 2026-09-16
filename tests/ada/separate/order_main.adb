with Order_A;
with Order_B;
with Ada.Text_IO;
procedure Order_Main is
begin
    Ada.Text_IO.Put_Line (Integer'Image (Order_B.Y));
end Order_Main;
