with Ada.Text_IO;
with Concat_Checks;
procedure Concatenation is
begin
    Concat_Checks.Run;
    Ada.Text_IO.Put_Line ("general concatenation");
end Concatenation;
