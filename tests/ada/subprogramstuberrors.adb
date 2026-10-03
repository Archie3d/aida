procedure SubprogramStubErrors is
    procedure Work is separate;
    function Value (Input : Integer) return Integer is SePaRaTe;
    procedure Completed;
    procedure Completed is separate;
    function "+" (Left, Right : Integer) return Integer is separate;

    package Nested is
        procedure Work;
        function Value return Integer;
    end Nested;
    package body Nested is
        procedure Work is separate;
        function Value return Integer is separate;
    end Nested;

    -- Reject stubs even in generic bodies that are never instantiated.
    generic
        type Item is private;
    package Generic_Package is
        procedure Work;
    end Generic_Package;
    package body Generic_Package is
        procedure Work is separate;
    end Generic_Package;

    generic
        type Item is private;
    function Generic_Function (Input : Item) return Item;
    function Generic_Function (Input : Item) return Item is separate;
begin
    declare
        procedure Block_Work is separate;
        function Block_Value return Integer is separate;
    begin
        null;
    end;
end SubprogramStubErrors;
