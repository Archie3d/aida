procedure AggregateTemporaries is
    type Values is array (1 .. 4) of Integer;
    type Any_Values is array (Integer range <>) of Integer;
    Items : Values := (1, 2, 3, 4);
    Calls : Integer := 0;

    function Next return Integer is
    begin
        Calls := Calls + 1;
        if Calls = 3 then
            raise Constraint_Error;
        end if;
        return 100 + Calls;
    end Next;

    Empty : Any_Values (4 .. 3) := (others => Next);
begin
    if Calls /= 0 or Empty'Length /= 0 then
        raise Program_Error;
    end if;
    for I in 1 .. 200 loop
        Items := (Items (4), Items (3), Items (2), Items (1));
    end loop;
    if Items /= (1, 2, 3, 4) then
        raise Program_Error;
    end if;
    begin
        Items := (others => Next);
        raise Program_Error;
    exception
        when Constraint_Error =>
            if Calls /= 3 or Items /= (1, 2, 3, 4) then
                raise Program_Error;
            end if;
    end;
end AggregateTemporaries;
