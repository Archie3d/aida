with Generic_Family.Child;
with Child_Support;
with Child_Parent;
procedure GenericChildErrors is
    package One is new Generic_Family (Integer, 1, Child_Support.Increment);
    package Two is new Generic_Family (Integer, 2, Child_Support.Increment);
    package A is new One.Child;
    package B is new Two.Child;
    generic
        with package Parent is new Generic_Family (<>);
        with package Operations is new Parent.Child (<>);
    package Verify is
    end Verify;
    package Bad_Parent is new Verify (One, B);
    X : One.Cell;
    Y : Two.Cell;
    Leak : Generic_Family.Cell;
    Hidden : Integer := Child_Parent.Touches;
begin
    A.Update (Y);
    X.Value := 3;
end GenericChildErrors;
