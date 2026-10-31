package body Operator_Visibility_Alias is
    function "+" (Right : Number) return Number is
    begin
        return 999;
    end "+";
    function "and" (Left : Number; Right : Boolean) return Number is
    begin
        return Left;
    end "and";
    procedure Verify is
    begin
        if Difference /= 1 or else +Number'(2) /= 999
            or else (Number'(3) and True) /= 3
            or else Operator_Visibility_Source."+" (2) /= 102 then
            raise Program_Error;
        end if;
    end Verify;
end Operator_Visibility_Alias;
