with Ada.Unchecked_Deallocation;
procedure Allocationcalls is
    type Pointer is access Integer;
    procedure Free is new Ada.Unchecked_Deallocation (Integer, Pointer);
    Total : Long_Integer := 0;
begin
    for I in 1 .. 100_000 loop
        declare
            Item : Pointer := new Integer'(I);
        begin
            Total := Total + Long_Integer (Item.all);
            Free (Item);
        end;
    end loop;
    if Total /= 5_000_050_000 then
        raise Program_Error;
    end if;
end Allocationcalls;
