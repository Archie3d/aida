package body Generic_Family.Secret is
    function Initial return Element is
    begin
        return Seed;
    end Initial;
    function Initial_Count (Value : Integer := Touches) return Integer is
    begin
        return Value;
    end Initial_Count;
end Generic_Family.Secret;
