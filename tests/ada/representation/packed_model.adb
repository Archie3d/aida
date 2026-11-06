package body Packed_Model is
    function Invert (Value : Bits) return Bits is
    begin
        return not Value;
    end Invert;
    procedure Toggle (Value : in out Bits) is
    begin
        for I in Value'Range loop
            Value (I) := not Value (I);
        end loop;
    end Toggle;
end Packed_Model;
