package OperatorLibrary is
    type Box is private;
    function Make (Value : Integer) return Box;
    function Value (Item : Box) return Integer;
    function "+" (Left, Right : Box) return Box;
    function "=" (Left, Right : Box) return Boolean;
private
    type Box is record
        Item : Integer;
    end record;
end OperatorLibrary;
