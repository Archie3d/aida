procedure Returnbodyerrors is
    package Inner is
    end Inner;
    package body Inner is
        procedure Legal is
        begin
            return;
        end Legal;
    begin
        return;
    exception
        when others =>
            return;
    end Inner;
    function Value return Integer is
    begin
        return Result : Integer := 0 do
            declare
                package Nested is
                end Nested;
                package body Nested is
                begin
                    return;
                exception
                    when others =>
                        return;
                end Nested;
            begin
                return;
            end;
        end return;
    end Value;
begin
    null;
end Returnbodyerrors;
