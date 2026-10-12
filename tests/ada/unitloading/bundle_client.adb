with Ada.Text_IO;
with Bundle_Consumer;
procedure Bundle_Client is
begin
    Ada.Text_IO.Put_Line (Integer'Image (Bundle_Consumer.Value));
end Bundle_Client;
