procedure IntegerLiteralErrors is
    Too_Large : Long_Integer := 170141183460469231731687303715884105728;
    Scaled_Too_Large : Long_Integer := 170141183460469231731687303715884105727E1;
    Exponent_Too_Large : Long_Integer := 2#1#E999999999999999999999999999999;
    Decimal_Exponent : Long_Integer := 1E999999999999999999999999999999;
    Zero_Exponent : Long_Integer := 2#0#E999999999999999999999999999999;
    Based_Overflow : Long_Integer := 16#80000000000000000000000000000000#;
    Based_Scaled : Long_Integer := 2#1#E127;
begin
    null;
end IntegerLiteralErrors;
