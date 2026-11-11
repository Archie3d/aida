with Ada.Finalization;
procedure Controlleddependentarrayerrors is
    type Cell is new Ada.Finalization.Controlled with null record;
    type Items is array (Positive range <>) of Cell;
    subtype Small is Natural range 0 .. 2;
    type Bundle (Count : Small) is record
        Data : Items (1 .. Count);
    end record;
    for Bundle use record
        Count at 0 range 0 .. 31;
        Data at 8 range 0 .. 127;
    end record;
begin
    null;
end Controlleddependentarrayerrors;
