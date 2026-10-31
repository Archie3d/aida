procedure FloatAttributeErrors is
    X : Float := 1.0;
    I : Integer;
    R : Float;
    B : Boolean;
    type Other is digits 6;
    O : Other := 1.0;
    package Hidden is
        type Real is private;
    private
        type Real is digits 6;
    end Hidden;
    Bad_Remainder : constant Float := Float'Remainder (1.0, 0.0);
    Bad_Part : constant Float := Float'Leading_Part (1.0, 0);
    Bad_Part_Negative : constant Float := Float'Leading_Part (1.0, -1);
    Bad_Machine : constant Float := Float'Machine (1.0E38 * 10.0);
    Bad_Skipped : constant Boolean := False and then Float'Remainder (1.0, 0.0) = 0.0;
    function Bad_Profile (Arg : Float) return Integer renames Float'Exponent;
begin
    R := X'Floor (X);
    R := Integer'Floor (1);
    R := Hidden.Real'Floor (1.0);
    R := Float'Floor;
    R := Float'Floor (X, X);
    R := Float'Floor (O);
    R := Float'Floor (I);
    R := Float'Compose (X, X);
    R := Float'Scaling (X);
    R := Float'Leading_Part (X, True);
    I := Float'Exponent (O);
    I := Float'Machine_Mantissa (X);
    B := X'Denorm;
    B := Integer'Signed_Zeros;
end FloatAttributeErrors;
