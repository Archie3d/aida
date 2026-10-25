with Operator_Visibility_Alias;
function Operator_Visibility_Client return Boolean is
    -- The declaring package is not otherwise visible in this compilation unit.
    use type Operator_Visibility_Alias.Number;
    X : Operator_Visibility_Alias.Number := 2;
begin
    return +X = 102 and then X ** 2 = 88 and then X + 3 = 5;
end Operator_Visibility_Client;
