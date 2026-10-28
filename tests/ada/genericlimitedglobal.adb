package body GenericLimitedGlobal is
    function Valid (Value : Cell) return Boolean is
    begin
        return Value.Value = 31 and Value.Self.all'Address = Value.Value'Address;
    end Valid;
end GenericLimitedGlobal;
