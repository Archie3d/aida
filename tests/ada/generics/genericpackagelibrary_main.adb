with Ada.Text_IO;
with GenericPackageModel;
with GenericPackageLibrary;
procedure GenericPackageLibrary_Main is
    procedure Check is
        Calls : Integer := 0;
        function Seed return Integer is
        begin
            Calls := Calls + 1;
            return 3;
        end Seed;
        Initial : constant Integer := Seed;
        package Data is new GenericPackageModel (Integer, Initial);
        package View is new GenericPackageLibrary (Data);
        package Nested is new View.Inner (Data);
        Object : Data.Cell := (Value => 7);
    begin
        if Calls /= 1 or View.Read /= 3 or Nested.Read /= 3 or View.Copy (Object).Value /= 7 then
            raise Program_Error;
        end if;
        Data.Write (9);
        View.Reset;
        if Data.Read /= 3 or Calls /= 1 then
            raise Program_Error;
        end if;
        begin
            View.Fail;
            raise Program_Error;
        exception
            when Data.Failure => null;
        end;
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
    Ada.Text_IO.Put_Line ("library formal packages passed");
end GenericPackageLibrary_Main;
