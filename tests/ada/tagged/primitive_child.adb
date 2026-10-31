package body Primitive_Child is
    overriding function "=" (X, Y : T) return Boolean is
    begin
        return X.Value mod 2 = Y.Value mod 2;
    end "=";
end Primitive_Child;
