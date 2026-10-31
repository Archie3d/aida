generic
    type P is access Integer range <>;
package Trailing is
end Trailing;
generic
    type P is access all constant Integer;
package Mixed_Modifiers is
end Mixed_Modifiers;
generic
    type P is not access Integer;
package Missing_Null is
end Missing_Null;
generic
    type P (<>) is access Integer;
package Discriminants is
end Discriminants;
