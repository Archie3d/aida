procedure SubprogramStubErrors is
    package Nested is
        procedure Work is separate;
    end Nested;

    procedure Enclosed is
        procedure Work is separate;
        package Local is
        end Local;
        package body Local is separate;
    begin
        null;
    end Enclosed;

    generic
        type Item is private;
    package Generic_Package is
        procedure Work is separate;
    end Generic_Package;

    function "+" (Left, Right : Integer) return Integer is separate;
begin
    declare
        procedure Block_Work is separate;
        package Block_Package is
        end Block_Package;
        package body Block_Package is separate;
    begin
        null;
    end;
end SubprogramStubErrors;
