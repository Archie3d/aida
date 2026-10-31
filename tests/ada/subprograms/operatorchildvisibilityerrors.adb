package Operator_Child_Source is
    type Number is new Integer;
end Operator_Child_Source;
with Operator_Child_Source;
package Operator_Parent is
private
    use type Operator_Child_Source.Number;
end Operator_Parent;
with Operator_Child_Source;
private package Operator_Parent.Private_Child is
    Value : Operator_Child_Source.Number := Operator_Child_Source.Number'(3) + 2;
end Operator_Parent.Private_Child;
with Operator_Child_Source;
package Operator_Parent.Public_Child is
    Bad : Operator_Child_Source.Number := Operator_Child_Source.Number'(3) + 2;
private
    Good : Operator_Child_Source.Number := Operator_Child_Source.Number'(3) + 2;
end Operator_Parent.Public_Child;
