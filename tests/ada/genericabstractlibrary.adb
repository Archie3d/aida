package body GenericAbstractLibrary is
    function Score (Object : T'Class) return Integer is
    begin
        return Measure (Object => Object);
    end Score;
    function Box_Score (Object : T'Class) return Integer is
    begin
        return Read (Object => Object);
    end Box_Score;
    procedure Update (Object : in out T'Class) is
    begin
        Change (Object => Object);
    end Update;
    function Nested_Score (Object : T'Class) return Integer is
    begin
        return Nested (Object => Object);
    end Nested_Score;
end GenericAbstractLibrary;
