procedure AccessSubtypeErrors is
    type Item (Left, Right : Positive) is null record;
    type Reference is access Item;
    subtype Fixed is Reference (1, 2);
    subtype Again is Fixed (1, 2);
    subtype Missing is Reference (1);
    subtype Duplicate is Reference (Left => 1, Left => 2);
    subtype Unknown is Reference (No_Such => 1, Right => 2);
    subtype Wrong is Reference (True, 2);
    subtype Range_Value is Reference (1 .. 2, 2);
    type Mutable (D : Integer := 1) is null record;
    type General is access all Mutable;
    subtype Unsafe is General (1);
    package Hidden is
        type Private_Item is private;
    private
        type Private_Item (D : Integer := 1) is null record;
        type Private_Reference is access Private_Item;
        subtype Illegal_Partial is Private_Reference (1);
    end Hidden;
    subtype One is Reference (1, 1);
    subtype Two is Reference (2, 2);
    procedure Conform (Value : One);
    procedure Conform (Value : Two) is
    begin
        null;
    end Conform;
    generic
        type Formal_Record (D : Integer) is private;
        type Formal_Access is access Formal_Record;
    package Generic_Constraints is
    end Generic_Constraints;
    package body Generic_Constraints is
        subtype Unsafe_Formal is Formal_Access (1);
    end Generic_Constraints;
begin
    null;
end AccessSubtypeErrors;
