pragma Suppress (All_Checks);
procedure PragmaUnsuppressed is
    pragma Unsuppress (All_Checks);
    subtype Small is Integer range 1 .. 3;
    Value : Integer := 4;
    type Pointer is access all Integer;
    P : Pointer;
    Result : Small;
begin
    begin
        Result := Value;
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    begin
        Result := P.all;
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
end PragmaUnsuppressed;
