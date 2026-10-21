package body Real_Edge_Checks is
    procedure Run is
        X : Real'Base := 2.0;
        Zero : Real'Base := 0.0;
        Negative_Zero : Real'Base := -1.0;
        Half : Real'Base := 0.5;
        Maximum : Real'Base := Real'Base'Last;
        Tiny : Real'Base := Real'Succ (0.0);
        Result : Real'Base;
        Exponent : Integer := -3;
        Minimum_Exponent : Integer := Integer'First;
        Maximum_Exponent : Integer := Integer'Last;
        Calls : Integer := 0;
        subtype Unit_Real is Real range -1.0 .. 1.0;
        Unit_Value : Unit_Real;
        function Base return Real'Base is
        begin
            Calls := Calls + 1;
            return X;
        end Base;
        function Power return Integer is
        begin
            Calls := Calls + 1;
            return Exponent;
        end Power;
    begin
        if not Real'Machine_Overflows or not Real'Machine_Rounds
            or 2.0 ** (-3) /= 0.125 or 0.0 ** 0 /= 1.0
            or 1.0 ** Integer'First /= 1.0 then
            raise Program_Error;
        end if;
        Result := Base ** Power;
        if Result /= 0.125 or Calls /= 2 or X / Half /= 4.0 then
            raise Program_Error;
        end if;
        X := -2.0;
        if X ** Exponent /= -0.125 or X ** 3 /= -8.0 or X ** (-2) /= 0.25 then
            raise Program_Error;
        end if;
        X := -1.0;
        if X ** Minimum_Exponent /= 1.0 or X ** Maximum_Exponent /= -1.0 then
            raise Program_Error;
        end if;
        X := 2.0;
        if Real'Base'Size = Float'Size then
            Exponent := -149;
        else
            Exponent := -1074;
        end if;
        if X ** Exponent /= Tiny or X ** (Exponent - 1) /= 0.0 then
            raise Program_Error;
        end if;
        Exponent := -3;
        if X ** Minimum_Exponent /= 0.0 or Zero ** 0 /= 1.0
            or Zero ** 3 /= 0.0 or Tiny ** 0 /= 1.0
            or Maximum ** 1 /= Maximum or Maximum ** (-1) <= 0.0
            or Maximum ** (-2) /= 0.0 or Tiny * Half /= 0.0 then
            raise Program_Error;
        end if;
        Negative_Zero := Negative_Zero * Zero;
        begin
            Result := X / Zero;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Result := X / Negative_Zero;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Result := Zero / Zero;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Result := Zero ** Exponent;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Result := Negative_Zero ** Minimum_Exponent;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Result := Tiny ** (-1);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Result := Half ** Minimum_Exponent;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Result := Maximum + Maximum;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Result := -Maximum - Maximum;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Result := (Maximum * X) / X;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Result := Maximum / Half;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Result := Maximum ** 2;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Unit_Value := Half ** (-1);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
    end Run;
end Real_Edge_Checks;
