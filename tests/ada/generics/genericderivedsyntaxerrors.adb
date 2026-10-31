generic
    type T is new Integer with null record;
package Concrete_Extension is
end Concrete_Extension;
generic
    type T is new Integer range 1 .. 3;
package Inline_Constraint is
end Inline_Constraint;
generic
    type T is tagged record
        Value : Integer;
    end record;
package Concrete_Record is
end Concrete_Record;
generic
    type T is new Integer and Some_Interface;
package Progenitors is
end Progenitors;
