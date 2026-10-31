package body Dispatch_Model is
    function Value (Item : Root) return Integer is
    begin
        return Item.X;
    end Value;
    function Value (Item : Child) return Integer is
    begin
        return Item.X + Item.Y;
    end Value;
    function Extra (Item : Child) return Integer is
    begin
        return Item.Y;
    end Extra;
    function Value (Item : Leaf) return Integer is
    begin
        return Item.X + Item.Y + 1;
    end Value;
    function Apply (Item : Root'Class) return Integer is
    begin
        return Value (Item);
    end Apply;
end Dispatch_Model;
