with Ada.Text_IO;
with Completion_Alias;
with Completion_Null;
with Completion_Empty;
with Completion_Instance;
procedure Completion_Main is
    Value : Integer := 21;
begin
    Completion_Null;
    Completion_Empty (Value);
    Ada.Text_IO.Put_Line (Integer'Image (Completion_Alias (Value)));
    Ada.Text_IO.Put_Line (Integer'Image (Completion_Instance (41)));
end Completion_Main;
