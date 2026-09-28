package Primitive_Base is
    type T is record
        Value : Integer;
    end record;
    function Make (Value : Integer := 2) return T;
    function Read (X : T) return Integer;
    function "=" (X, Y : T) return Boolean;
end Primitive_Base;
