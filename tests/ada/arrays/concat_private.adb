package body Concat_Private is
    function Make (Value : Integer) return Item is
    begin
        return (Value => Value);
    end Make;
    function Number (Value : Item) return Integer is
    begin
        return Value.Value;
    end Number;
end Concat_Private;
