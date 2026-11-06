with Ada.Text_IO; use Ada.Text_IO;
with General_Freezing;
procedure GeneralFreezing is
    task type Worker;
    task body Worker is
    begin
        General_Freezing.Check;
        Put_Line ("general freezing OK");
    end Worker;
    Workers : array (1 .. 4) of Worker;
begin
    General_Freezing.Check;
end GeneralFreezing;
