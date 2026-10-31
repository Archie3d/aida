package body Scalar_Dynamic_Model is
    function Echo (Value : Small) return Small is
    begin
        return Value;
    end Echo;
    function Real_Echo (Value : Real_Range) return Real_Range is
    begin
        return Value;
    end Real_Echo;
begin
    Limit := 100;
    Real_Limit := 100.0;
    Fixed_Limit := 10.0;
end Scalar_Dynamic_Model;
