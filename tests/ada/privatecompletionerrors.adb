procedure PrivateCompletionErrors is
    package Defaulted is
        type Item (N : Integer := 1) is tagged private;
    private
        type Item (N : Integer := 1) is tagged null record;
    end Defaulted;
    package Indefinite is
        type Item is tagged private;
    private
        type Item (N : Positive) is tagged null record;
    end Indefinite;
    package Limited_View is
        type Item is tagged limited private;
    private
        type Item is tagged null record;
    end Limited_View;
    package Mismatched is
        type Item (N : Positive) is tagged private;
    private
        type Item (M : Positive) is tagged null record;
    end Mismatched;
    package Constraints is
        type Root (N : Positive) is tagged null record;
        type Child is new Root (3) with private;
    private
        type Child is new Root (4) with null record;
    end Constraints;
    package Keywords is
        type Root is tagged limited null record;
        type Child is limited new Root with private;
    private
        type Child is new Root with null record;
    end Keywords;
    package Frozen is
        type Item is private;
        type Outer is tagged record
            Value : Item;
        end record;
        Object : Outer;
    private
        type Item is record
            Value : Integer;
        end record;
    end Frozen;
    package Nonlimited_Full_View is
        type Item is limited private;
        type Link is access all Item;
    private
        type Item is record
            Self : Link := Item'Access;
        end record;
    end Nonlimited_Full_View;
begin
    null;
end PrivateCompletionErrors;
