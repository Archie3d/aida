with Ada.Text_IO;
with GenericInterfaceModel;
with GenericInterfaceLibrary;
procedure GenericInterfaceLibrary_Main is
    package First is new GenericInterfaceLibrary (GenericInterfaceModel.Both, GenericInterfaceModel.Item);
    package Second is new GenericInterfaceLibrary (GenericInterfaceModel.Base, GenericInterfaceModel.Item);
    procedure Check is
        X : GenericInterfaceModel.Item;
    begin
        if First.Static_Read (X) /= 43 or First.Dynamic_Read (X) /= 43
            or Second.Static_Read (X) /= 43 or Second.Dynamic_Read (X) /= 43 then
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
    Ada.Text_IO.Put_Line ("library generic interface contracts passed");
end GenericInterfaceLibrary_Main;
