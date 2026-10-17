package body Derived_Model is
    function Make (Value : Integer) return Number is
    begin
        return Number (Value);
    end Make;
    function Read (Value : Number) return Integer is
    begin
        return Integer (Value);
    end Read;
end Derived_Model;
