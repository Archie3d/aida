procedure Checkedmultiply is
    Value : Long_Integer := 1;
    Factor : Long_Integer := 1;
begin
    for I in 1 .. 10_000_000 loop
        Value := (Value * Factor + 7) rem 1009;
        Factor := Factor rem 17 + 1;
    end loop;
    if Value /= 234 or else Factor /= 6 then
        raise Program_Error;
    end if;
end Checkedmultiply;
