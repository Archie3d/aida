package body Context_Client is
    function Read (Object : Data) return Integer is
    begin
        return Object.Value.Value + Context_Secret.Read;
    end Read;
    package body Nested is
        function Read return Integer is
        begin
            return Value.Value;
        end Read;
    end Nested;
end Context_Client;
