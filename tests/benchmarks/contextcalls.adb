procedure Contextcalls is
    function Next (Value : Integer) return Integer is
    begin
        return Value + 1;
    end Next;
    Value : Integer := 0;
begin
    for I in 1 .. 2_000_000 loop
        Value := Next (Value);
    end loop;
    if Value /= 2_000_000 then
        raise Program_Error;
    end if;
end Contextcalls;
