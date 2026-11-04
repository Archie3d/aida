package body Freezing_Layout is
    function Make return Item_View is
    begin
        return (10, 20);
    end Make;
    function Sum (Value : Item_View) return Integer is
    begin
        return Value.First + Value.Second;
    end Sum;
    function Chosen (Value : Integer) return Integer is
    begin
        return Value;
    end Chosen;
end Freezing_Layout;
