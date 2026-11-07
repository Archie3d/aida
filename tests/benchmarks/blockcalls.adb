procedure Blockcalls is
    type Values is array (1 .. 4) of Integer;
    Total : Long_Integer := 0;
begin
    for I in 1 .. 200_000 loop
        declare
            Local : Integer := I;
            Items : Values := (others => Local);
        begin
            Total := Total + Long_Integer (Items (2));
        end;
    end loop;
    if Total /= 20_000_100_000 then
        raise Program_Error;
    end if;
end Blockcalls;
