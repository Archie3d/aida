procedure GenericNullMatchingErrors is
    generic
        with procedure Hook (Object : in out Integer) is null;
    package Views is
    end Views;
    procedure Read_Only (Object : Integer) is null;
    function Read (Object : Integer) return Integer is
    begin
        return Object;
    end Read;
    package Wrong_Mode is new Views (Read_Only);
    package Wrong_Kind is new Views (Read);
    package Null_Actual is new Views (null);
begin
    null;
end GenericNullMatchingErrors;
