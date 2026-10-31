with Ada.Text_IO;
with GenericAccessObjectLibrary;
procedure GenericAccessObjectLibrary_Main is
    procedure Check is
        A : aliased Integer := 17;
        B : aliased Integer := 25;
        P : access Integer := A'Access;
        package Instance is new GenericAccessObjectLibrary (Integer, P, P);
        function Nested is new Instance.Read_Other;
        function Explicit is new Instance.Read_Other (B'Access);
    begin
        Instance.Set (B'Access);
        if A /= 25 or Instance.Read /= 25 or Nested /= 25 or Explicit /= 25 then
            raise Program_Error;
        end if;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check;
    end Worker;
begin
    Check;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("access object library passed");
end GenericAccessObjectLibrary_Main;
