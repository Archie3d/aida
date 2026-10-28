package body GenericContractLibrary is
    function Score (Object : T'Class) return Integer is
    begin
        return Read (Object);
    end Score;
    function Nested_Score (Object : T'Class) return Integer is
    begin
        return Measure (Object);
    end Nested_Score;
end GenericContractLibrary;
