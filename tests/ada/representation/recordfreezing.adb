with Ada.Text_IO; use Ada.Text_IO;
with Freezing_Layout; use Freezing_Layout;
procedure RecordFreezing is
    type Local is record
        Head : Boolean;
        Tail : Integer;
    end record;
    subtype View is Local;
    Offset : constant := 4;
    for Local use record
        Head at 0 range 1 .. 1;
        Tail at Offset range 0 .. 31;
    end record;
    for Local'Size use 128;

    procedure Check is
        Public_Item : Item_View := Make;
        Pointer : Item_Pointer := new Item'(Make);
        Data : View := (True, 33);
    begin
        if Public_Item.First'Position /= 4 or else Public_Item.Second'Position /= 0
            or else Public_Item.First /= 10 or else Pointer.Second /= 20
            or else Item_View'Size /= 128 or else Item'Alignment /= 8
            or else not Data.Head or else Data.Head'First_Bit /= 1
            or else Data.Tail /= 33 or else View'Size /= 128 then
            raise Program_Error;
        end if;
    end Check;
begin
    Check;
    declare
        task type Worker;
        task body Worker is
        begin
            Check;
            Put_Line ("record freezing OK");
        end Worker;
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
end RecordFreezing;
