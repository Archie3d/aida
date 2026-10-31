with Rejection_Model.Hidden;
package body Rejection_Model is
    function Create (Length : Positive) return Item is
    begin
        return (Length, 123);
    end Create;
    function Make_Secret return Secret is
    begin
        return (Key => 7, Value => 1);
    end Make_Secret;
    function Check return Boolean is
        Value : Name := Hidden.Join;
    begin
        return Value = "abYZ";
    end Check;
    function "+" (Left : Tagged_Item; Right : Integer) return Tagged_Item is
    begin
        return (Value => Left.Value + Right);
    end "+";
    function Values return Integer_Array is
    begin
        return (5 .. 7 => 9);
    end Values;
end Rejection_Model;
