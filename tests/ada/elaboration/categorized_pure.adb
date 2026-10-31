package body Categorized_Pure is
    function Compute return Integer is
    begin
        return 99;
    end Compute;
    function Increment (Value : Integer) return Integer is
        Local : Integer := Value;
        type Local_Reference is access Integer;
    begin
        Local := Local + 1;
        return Local;
    end Increment;
begin
    pragma Assert (True);
end Categorized_Pure;
