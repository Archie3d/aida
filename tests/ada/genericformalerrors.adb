generic
    type T is mod <>;
package Modular_Formal is
end Modular_Formal;

generic
    type T is new Integer;
package Derived_Formal is
end Derived_Formal;

generic
    type T is access Integer;
package Access_Formal is
end Access_Formal;

generic
    type T is tagged private;
package Tagged_Formal is
end Tagged_Formal;

generic
    type T is abstract tagged limited private;
package Abstract_Limited_Formal is
end Abstract_Limited_Formal;

generic
    type T is range <> mod <>;
package Range_Trailing_Formal is
end Range_Trailing_Formal;

generic
    type T is digits <> range <>;
package Digits_Trailing_Formal is
end Digits_Trailing_Formal;
