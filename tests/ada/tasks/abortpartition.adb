with Ada.Text_IO; use Ada.Text_IO;
with Ada.Task_Identification; use Ada.Task_Identification;
procedure AbortPartition is
    Ready : Boolean := False;
    pragma Atomic (Ready);
    task Child;
    task body Child is
    begin Ready := True; loop delay 60.0; end loop; end Child;
begin
    while not Ready loop delay 0.0; end loop;
    Put_Line ("aborting partition");
    Abort_Task (Current_Task);
    Put_Line ("continued after abort");
end AbortPartition;
