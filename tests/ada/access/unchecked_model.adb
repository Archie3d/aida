package body Unchecked_Model is
    function Read (Object : Root) return Integer is
    begin
        return Object.Value;
    end Read;
    function Access_Read (Object : access Root) return Integer is
    begin
        return Object.Value;
    end Access_Read;
    function Access_Read (Object : access Child) return Integer is
    begin
        return Object.Value + Object.Extra;
    end Access_Read;
end Unchecked_Model;
