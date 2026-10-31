package body Categorized_Types is
    protected body Lock is
        procedure Touch is
        begin
            Count := Count + 1;
        end Touch;
    end Lock;
    function Get (Item : Value) return Integer is
    begin
        return Item.Count;
    end Get;
begin
    null;
end Categorized_Types;
