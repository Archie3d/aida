procedure Checkedarithmetic is
    Value : Integer := 1;
    Wide : Long_Integer := 1;
begin
    for I in 1 .. 10_000_000 loop
        Value := (Value + 7) * 3 rem 1009;
        Wide := (Wide + 7) * 3 rem 1009;
    end loop;
    if Value /= 977 or else Wide /= 977 then
        raise Program_Error;
    end if;
end Checkedarithmetic;
