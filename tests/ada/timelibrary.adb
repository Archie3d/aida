with Ada.Text_IO;
with Timed_Model;
procedure TimeLibrary is
begin
    Timed_Model.Notice.Wait;
    Ada.Text_IO.Put_Line ("time library passed");
end TimeLibrary;
