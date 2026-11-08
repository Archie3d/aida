procedure BlockMasterCalls is
    function Runtime_Looking (Index, First, Last, Stride : Long_Integer)
        return Long_Integer;
    pragma Import (C, Runtime_Looking, "__ada_array_offset");

    procedure Touch (Value : in out Integer) is
    begin
        Value := Value + 1;
    end Touch;
begin
    declare
        Value : Integer := 1;
    begin
        Touch (Value);
        declare
            Copy : Integer := Value;
        begin
            Value := Copy;
        end;
        if Value /= 2 then
            raise Program_Error;
        end if;
    end;
    declare
        Value : Long_Integer := Runtime_Looking (2, 1, 4, 4);
    begin
        if Value /= 4 then
            raise Program_Error;
        end if;
    end;
end BlockMasterCalls;
