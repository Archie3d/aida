package Real_Number_Model is
    Factor : constant := 2 ** 100;
    Product : constant := Factor * 0.125;
    Quotient : constant := Product / Factor;
    Converted_Real : constant := Float (1.25);
    Converted_Integer : constant := Long_Integer (6);
    type Fraction is delta 0.125 range -8.0 .. 8.0;
    Converted_Fixed : constant := Fraction (1.4);
end Real_Number_Model;
