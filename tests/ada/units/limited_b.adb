package body Limited_B is
    function Read (Object : Node) return Integer is
    begin
        return Object.Value;
    end Read;
end Limited_B;
