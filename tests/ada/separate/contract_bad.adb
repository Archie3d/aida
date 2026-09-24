package body Contract_Bad is
    function Add (L, R : Item) return Item is
    begin
        return L + R;
    end Add;
end Contract_Bad;
