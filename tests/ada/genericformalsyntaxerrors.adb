generic
    type T is range;
package Bad_Form_0 is
end Bad_Form_0;

generic
    type T is digits 6;
package Bad_Form_1 is
end Bad_Form_1;

generic
    type T is (Red, Blue);
package Bad_Form_2 is
end Bad_Form_2;

generic
    type T is (<>) range <>;
package Bad_Form_3 is
end Bad_Form_3;

generic
    type T is private new Integer;
package Bad_Form_4 is
end Bad_Form_4;

generic
    type T is limited tagged private;
package Bad_Form_5 is
end Bad_Form_5;

generic
    type T is delta <> digits <>;
package Bad_Form_6 is
end Bad_Form_6;

generic
    type T is array (Integer range <>) of Integer range <>;
package Bad_Form_7 is
end Bad_Form_7;
