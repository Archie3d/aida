with Ada.Integer_Text_IO;
with Ada.Text_IO;
with Incrhelper;

procedure Incremental is
begin
    Ada.Integer_Text_IO.Put (Incrhelper.Value, 1);
    Ada.Text_IO.New_Line;
end Incremental;
