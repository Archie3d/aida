package body OperatorLibrary is
    function Make (Value : Integer) return Box is
    begin
        return (Item => Value);
    end Make;
    function Value (Item : Box) return Integer is
    begin
        return Item.Item;
    end Value;
    function "+" (Left, Right : Box) return Box is
    begin
        return (Item => Left.Item + Right.Item);
    end "+";
    function "=" (Left, Right : Box) return Boolean is
    begin
        return Left.Item mod 10 = Right.Item mod 10;
    end "=";
end OperatorLibrary;
