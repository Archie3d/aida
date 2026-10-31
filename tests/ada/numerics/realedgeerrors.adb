procedure RealEdgeErrors is
    Bad_Divisor : Float := 2.0 / 0.0;
    Bad_Zero : Long_Float := 0.0 / (-0.0);
    Bad_Power : Float := 0.0 ** (-3);
    Bad_First : Long_Float := 0.0 ** Integer'First;
    Wide_Exponent : Long_Integer := -3;
    X : Float := 2.0;
    Bad_Type : Float := X ** Wide_Exponent;
    Bad_Prefix : Boolean := Integer'Machine_Overflows;
    Bad_Arity : Boolean := Float'Machine_Rounds (1);
    Bad_Object : Boolean := X'Machine_Overflows;
begin
    null;
end RealEdgeErrors;
