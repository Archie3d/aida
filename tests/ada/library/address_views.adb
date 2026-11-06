package body Address_Views is
    function Create (Value : Integer) return Hidden is
    begin
        return (Data => Value);
    end Create;
    function Value (Item : Hidden) return Integer is
    begin
        return Item.Data;
    end Value;
end Address_Views;
