procedure FixedOperatorErrors is
    type A is delta 0.125 range -10.0 .. 10.0;
    type B is delta 0.25 range -10.0 .. 10.0;
    function "*" (Left, Right : A) return A is
    begin
        return Left + Right;
    end "*";
    X : A := 1.0;
    Y : B := X * X;
    Long : Long_Integer := 2;
    Z : A := X * Long;
    Too_Big : A := X * 2147483648;
begin
    null;
end FixedOperatorErrors;
