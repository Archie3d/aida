procedure IntegerLiteralErrors is
    Too_Large : Long_Integer := 9223372036854775808;
    Scaled_Too_Large : Long_Integer := 9223372036854775807E1;
    Exponent_Too_Large : Long_Integer := 2#1#E999999999999999999999999999999;
    Decimal_Exponent : Long_Integer := 1E999999999999999999999999999999;
    Zero_Exponent : Long_Integer := 2#0#E999999999999999999999999999999;
    Based_Overflow : Long_Integer := 16#8000000000000000#;
    Based_Scaled : Long_Integer := 2#1#E63;
begin
    null;
end IntegerLiteralErrors;
