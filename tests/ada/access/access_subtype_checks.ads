package Access_Subtype_Checks is
    type Item (Left, Right : Positive) is record
        Value : Integer := Left + Right;
    end record;
    type Reference is access all Item;
    Limit : Positive := 2;
    subtype Fixed is Reference (Left => Limit, Right => Limit);
    subtype Alias is Fixed;
    type Derived is new Fixed;
    Good : aliased Item (2, 2);
    Bad : aliased Item (3, 3);
    type Secret (D : Positive) is private;
    type Secret_Reference is access all Secret;
    subtype Secret_Fixed is Secret_Reference (Limit);
    function Make_Secret (Wrong : Boolean) return Secret_Reference;
    procedure Run;
private
    type Secret (D : Positive) is null record;
end Access_Subtype_Checks;
