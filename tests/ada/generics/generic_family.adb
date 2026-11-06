package body Generic_Family is
    function Get (Value : Cell) return Element is
    begin
        return Value.Value;
    end Get;
    function Count return Integer is
    begin
        return Touches;
    end Count;
end Generic_Family;
