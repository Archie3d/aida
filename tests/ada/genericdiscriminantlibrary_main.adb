with Ada.Text_IO;
with GenericDiscriminantLibrary;
procedure GenericDiscriminantLibrary_Main is
    type Pair (Low, High : Integer) is record
        First, Last : Integer;
    end record;
    package View is new GenericDiscriminantLibrary (Integer, Pair);
    procedure Check is
        X : Pair := (2, 7, 90, 91);
        Y : Pair := (7, 2, 90, 91);
    begin
        if View.Score (X) /= 9 or not View.Matches (X) or View.Matches (Y)
            or X.First /= 90 or X.Last /= 91 then
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
    Ada.Text_IO.Put_Line ("library generic discriminants passed");
end GenericDiscriminantLibrary_Main;
