package body Context_Client.Public is
    function Read return Integer is
    begin
        return Value.Value + Context_Client.Hidden.Value.Value;
    end Read;
end Context_Client.Public;
