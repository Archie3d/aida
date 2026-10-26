with Ada.Text_IO;
with GenericDerivedModel;
with GenericDerivedLibrary;
procedure GenericDerivedLibrary_Main is
    package Child_View is new GenericDerivedLibrary (GenericDerivedModel.Child);
    package Root_View is new GenericDerivedLibrary (GenericDerivedModel.Root);
    procedure Check is
    begin
        if Child_View.Read_Ancestor (3) /= 4 or Root_View.Read_Ancestor (5) /= 6 then
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
    Ada.Text_IO.Put_Line ("library generic derived types passed");
end GenericDerivedLibrary_Main;
