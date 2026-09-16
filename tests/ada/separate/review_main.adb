with Review_Generic;
with Ada.Text_IO;
procedure Review_Main is
    package Instance is new Review_Generic;
begin
    Ada.Text_IO.Put_Line (Integer'Image (Instance.Value));
end Review_Main;
