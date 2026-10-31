procedure RealNumberErrors is
    Zero_Divisor : constant := 1.0 / 0;
    Overflow : constant := (2.0 ** 100) * (2 ** 100);
    Denominator_Limit : constant := (1.0 / (2 ** 100)) / (2 ** 100);
    Mixed_Add : constant := 1.0 + 2;
    Mixed_Subtract : constant := 2 - 1.0;
    Reverse_Division : constant := 2 / 1.0;
    Real_Mod : constant := 1.0 mod 2;
    Real_Exponent : constant := 2.0 ** 1.0;
    Flag : constant := True;
    Dynamic : Float := 1.5;
    Nonstatic : constant := Dynamic * 2.0;
    Typed_Integer : constant Integer := 2;
    Typed_Real : constant Float := 1.5;
    Mixed_Typed : constant := Typed_Integer * Typed_Real;
    Root_With_Typed : constant := Typed_Real / Typed_Integer;
    type Small is digits 6 range 0.0 .. 1.0;
    Bad_Conversion : constant := Small (1.5);
    Bad_Qualification : constant := Small'(1.5);
begin
    null;
end RealNumberErrors;
