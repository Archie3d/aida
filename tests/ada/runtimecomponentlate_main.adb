with Ada.Text_IO;
with RuntimeComponentLate;
procedure RuntimeComponentLate_Main is
    A, B : RuntimeComponentLate.Outer;
begin
    A.Tail := 17;
    B := A;
    RuntimeComponentLate.N := 100;
    if B.Tail /= 17 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("delayed runtime components passed");
end RuntimeComponentLate_Main;
