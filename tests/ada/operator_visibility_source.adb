package body Operator_Visibility_Source is
    function "+" (Right : Number) return Number is
    begin
        return Number (Integer (Right) + 100);
    end "+";
    function "**" (Left : Number; Right : Integer) return Number is
    begin
        return 88;
    end "**";
    function Make (Value : Integer) return Token is
    begin
        return Token (Value);
    end Make;
end Operator_Visibility_Source;
