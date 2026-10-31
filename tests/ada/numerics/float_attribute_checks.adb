package body Float_Attribute_Checks is
    procedure Run is
        X : Real'Base := -2.5;
        Y : Real'Base := 2.0;
        Zero : Real'Base := 0.0;
        Negative_Zero : Real'Base := -0.0;
        Maximum : Real'Base := Real'Base'Last;
        Tiny : Real'Base := Real'Succ (0.0);
        Small : Real'Base := Real'Model_Small;
        Epsilon : Real'Base := Real'Model_Epsilon;
        Safe_First : Real'Base := Real'Safe_First;
        Safe_Last : Real'Base := Real'Safe_Last;
        Result : Real'Base;
        N : Integer := 3;
        Bad_Digits : Integer := 0;
        type Large_Count is mod 2 ** 64;
        Large : Large_Count := Large_Count'Last;
        Limit : Real'Base := 1.0;
        subtype Dynamic is Real range -Limit .. Limit;
        Calls : Integer := 0;
        Check_Number : Integer := 0;
        subtype Unit is Real range -1.0 .. 1.0;
        Constrained_Result : Unit;
        function Operand return Real'Base is
        begin
            Calls := Calls + 1;
            return X;
        end Operand;
        function Count return Integer is
        begin
            Calls := Calls + 1;
            return N;
        end Count;
        procedure Check (Condition : Boolean) is
        begin
            Check_Number := Check_Number + 1;
            if not Condition then
                raise Program_Error with "floating attribute check" & Integer'Image (Check_Number);
            end if;
        end Check;
        function Rounded (Arg : Real'Base) return Real'Base renames Real'Floor;
        generic
            with function Operation (Arg : Real'Base) return Real'Base;
        function Apply (Arg : Real'Base) return Real'Base;
        function Apply (Arg : Real'Base) return Real'Base is
        begin
            return Operation (Arg);
        end Apply;
        function Generic_Round is new Apply (Real'Ceiling);
    begin
        Check (Real'Machine_Radix = 2 and Real'Denorm and Real'Signed_Zeros);
        Check (Real'Model_Mantissa = Real'Machine_Mantissa);
        Check (Real'Model_Emin = Real'Machine_Emin);
        Check (Real'Machine_Emax > 0 and Real'Machine_Emin < 0);
        Check (Safe_First = -Maximum and Safe_Last = Maximum);
        Check (Small > Tiny and Epsilon > 0.0);
        Check (Real'Floor (X) = -3.0 and Real'Ceiling (X) = -2.0);
        Check (Real'Rounding (X) = -3.0 and Real'Truncation (X) = -2.0);
        Check (Real'Unbiased_Rounding (X) = -2.0 and Real'Machine_Rounding (X) = -2.0);
        Check (Real'Floor (-X) = 2.0 and Real'Ceiling (-X) = 3.0);
        Check (Real'Rounding (-X) = 3.0 and Real'Unbiased_Rounding (-X) = 2.0);
        Check (Real'Exponent (X) = 2 and Real'Fraction (X) = -0.625);
        Check (Real'Compose (X, N) = -5.0 and Real'Scaling (X, N) = -20.0);
        Check (Real'Leading_Part (X, 2) = -2.0);
        Check (Real'Leading_Part (X, Integer'Last) = X);
        Check (Real'Leading_Part (X, Large) = X);
        Check (Real'Leading_Part (X, 2 ** 64) = X);
        Check (Dynamic'Floor (X) = -3.0 and Dynamic'Machine_Radix = 2);
        Check (Real'Remainder (X, Y) = -0.5);
        Check (Real'Remainder (3.0 * Y, 2.0 * Y) = -Y);
        Check (Real'Adjacent (Zero, Y) = Tiny);
        Check (Real'Adjacent (Tiny, Zero) = Zero);
        Check (Real'Copy_Sign (X, Y) = 2.5);
        Check (Real'Copy_Sign (Y, Negative_Zero) = -Y);
        Check (Real'Copy_Sign (Y, Real'Floor (Negative_Zero)) = -Y);
        Check (Real'Copy_Sign (Y, Real'Ceiling (-Tiny)) = -Y);
        Check (Real'Copy_Sign (Y, Real'Truncation (-Tiny)) = -Y);
        Check (Real'Copy_Sign (Y, Real'Adjacent (Negative_Zero, Zero)) = -Y);
        Check (Real'Machine (X) = X and Real'Model (X) = X);
        Check (Real'Compose (Real'Fraction (Tiny), Real'Exponent (Tiny)) = Tiny);
        Check (Real'Scaling (Y, Integer'First) = Zero);
        Check (Rounded (X) = -3.0 and Generic_Round (X) = -2.0);
        Check (Unit'Floor (X) = -3.0);
        Result := Real'Scaling (Operand, Count);
        Check (Calls = 2 and Result = -20.0);
        begin
            Result := Real'Remainder (X, Zero);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Result := Real'Leading_Part (X, Bad_Digits);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Result := Real'Adjacent (Maximum, Maximum);
            Check (Result = Maximum);
            Result := Real'Scaling (Maximum, N);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Result := Real'Scaling (Y, Large);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Result := Real'Scaling (Y, Integer'Last);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Constrained_Result := Unit'Floor (X);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
    end Run;
end Float_Attribute_Checks;
