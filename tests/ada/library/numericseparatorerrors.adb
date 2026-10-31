procedure NumericSeparatorErrors is

    Valid          : constant Integer := 1_000 + 16#FF_FF# + 2#1#E1_0;
    Doubled        : constant Integer := 1__0;
    Trailing       : constant Integer := 1_;
    Before_Point   : constant Float   := 1_.5;
    Fraction_End   : constant Float   := 1.5_;
    Exponent_End   : constant Integer := 1E1_;
    Based_Leading  : constant Integer := 16#_FF#;
    Based_Trailing : constant Integer := 16#FF_#;
    Base_Trailing  : constant Integer := 16_#FF#;
    Based_Exponent : constant Integer := 2#1#E1__0;

begin
    null;
end NumericSeparatorErrors;
