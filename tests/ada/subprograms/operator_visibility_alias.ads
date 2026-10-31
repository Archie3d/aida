with Operator_Visibility_Source;
package Operator_Visibility_Alias is
    use Operator_Visibility_Source;
    subtype Number is Operator_Visibility_Source.Number;
    function "+" (Right : Number) return Number;
    procedure Verify;
private
    Difference : Number := Number'(3) - Number'(2);
    function "and" (Left : Number; Right : Boolean) return Number;
end Operator_Visibility_Alias;
