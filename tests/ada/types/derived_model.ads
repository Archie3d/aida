package Derived_Model is
    type Number is private;
    function Make (Value : Integer) return Number;
    function Read (Value : Number) return Integer;
    type Color is (Red, Green, Blue);
    subtype Middle is Color range Green .. Green;
private
    type Number is new Integer range 0 .. 100;
end Derived_Model;
