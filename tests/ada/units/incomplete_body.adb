package body Incomplete_Body is
    type Node is record
        Value : Integer := 31;
    end record;
    function Read return Integer is
        Object : Pointer := new Node;
    begin
        return Object.Value;
    end Read;
end Incomplete_Body;
