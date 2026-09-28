package body Primitive_Base is
    function Make (Value : Integer := 2) return T is
    begin
        return (Value => Value);
    end Make;
    function Read (X : T) return Integer is
    begin
        return X.Value;
    end Read;
    function "=" (X, Y : T) return Boolean is
    begin
        return X.Value = Y.Value;
    end "=";
end Primitive_Base;
