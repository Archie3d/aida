procedure EmptyMasterBlocks is
    type Values is array (1 .. 4) of Integer;
    Total : Integer := 0;

    function Value (I : Integer) return Integer is
    begin
        declare
            Local : Integer := I;
        begin
            if Local < 0 then
                raise Constraint_Error;
            elsif Local = 0 then
                return 7;
            end if;
            declare
                Other : Integer := Local + 1;
            begin
                return Other;
            end;
        end;
    exception
        when Constraint_Error =>
            return 11;
    end Value;
begin
    for I in 1 .. 4 loop
        declare
            Items : Values := (others => I);
        begin
            declare
                Local : Integer := Items (2);
            begin
                if I = 2 then
                    goto Done;
                end if;
                Total := Total + Local;
                <<Done>>
                null;
            end;
        end;
    end loop;
    if Total /= 8 or else Value (0) /= 7 or else Value (3) /= 4
        or else Value (-1) /= 11 then
        raise Program_Error;
    end if;
end EmptyMasterBlocks;
