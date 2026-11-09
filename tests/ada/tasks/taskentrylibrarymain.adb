with Ada.Text_IO; use Ada.Text_IO;
with TaskEntryLibrary; use TaskEntryLibrary;
procedure TaskEntryLibraryMain is
    A : Worker := Make;
    type Handle is access Worker;
    P : Handle := new Worker;
    N : Integer := 0;
begin
    A.Add (N);
    A.Stop;
    P.Add (N);
    P.Stop;
    if N /= 10 then raise Program_Error; end if;
    Put_Line ("library entries OK");
end TaskEntryLibraryMain;
