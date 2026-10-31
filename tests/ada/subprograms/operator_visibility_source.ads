package Operator_Visibility_Source is
    type Number is new Integer;
    function "+" (Right : Number) return Number;
    function "**" (Left : Number; Right : Integer) return Number;
    type Token is private;
    function Make (Value : Integer) return Token;
private
    type Token is new Integer;
end Operator_Visibility_Source;
