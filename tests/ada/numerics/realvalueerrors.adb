procedure RealValueErrors is
    X : Float := 0.0;
    Bad_Object : Float := X'Value ("1.0");
    Bad_Arity : Float := Float'Value ("1.0", "2.0");
    Bad_Argument : Float := Float'Value (1);
begin
    null;
end RealValueErrors;
