with Ada.Text_IO;
with Float_Attribute_Checks;
procedure FloatAttributes is
    procedure Check_Float is new Float_Attribute_Checks.Run (Float);
    procedure Check_Long is new Float_Attribute_Checks.Run (Long_Float);
    type Derived is new Long_Float;
    procedure Check_Derived is new Float_Attribute_Checks.Run (Derived);
    subtype Small is Long_Float range -1.0 .. 1.0;
    Rounded : constant := Small'Floor (-2.5);
    Exact_Floor : constant := Long_Float'Floor (0.99999999999999999999);
    Exact_Ceiling : constant := Long_Float'Ceiling (1.00000000000000000001);
    Exact_Rounding : constant := Long_Float'Rounding (0.49999999999999999999);
    Negative_Zero : constant Long_Float := Long_Float'Truncation (-0.25);
    Converted : Long_Float := Long_Float'Copy_Sign (1.0, Negative_Zero);
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error with "static floating attribute check";
        end if;
    end Check;
begin
    Check (Rounded = -3.0 and Exact_Floor = 0.0 and Exact_Ceiling = 2.0 and Exact_Rounding = 0.0);
    Check (Float'Machine_Radix = 2 and Float'Machine_Mantissa = 24 and Long_Float'Machine_Mantissa = 53);
    Check (Float'Machine_Emin = -125 and Long_Float'Machine_Emin = -1021);
    Check (Float'Machine_Emax = 128 and Long_Float'Machine_Emax = 1024);
    Check (Float'Model_Mantissa = 24 and Long_Float'Model_Mantissa = 53);
    Check (Float'Model_Emin = -125 and Long_Float'Model_Emin = -1021);
    Check (Float'Denorm and Float'Signed_Zeros);
    Check (Float'Model_Epsilon = 2.0 ** (-23) and Long_Float'Model_Epsilon = 2.0 ** (-52));
    Check (Float'Model_Small > 0.0 and Long_Float'Model_Small > 0.0);
    Check (Float'Safe_First < 0.0 and Long_Float'Safe_Last > 0.0);
    Check (Float'Exponent (3.0) = 2 and Float'Fraction (3.0) = 0.75);
    Check (Float'Compose (3.0, 4) = 12.0 and Float'Scaling (3.0, 4) = 48.0);
    Check (Float'Leading_Part (3.75, 2) = 3.0);
    Check (Float'Remainder (7.0, 2.0) = -1.0);
    Check (Float'Remainder (0.1, 1.0) = 0.0);
    Check (Float'Unbiased_Rounding (3.5) = 4.0 and Float'Unbiased_Rounding (2.5) = 2.0);
    Check (Float'Machine_Rounding (-3.5) = -4.0);
    Check (Float'Machine (1.0 + 2.0 ** (-24)) = 1.0);
    Check (Float'Model (1.0 + 2.0 ** (-24)) = 1.0);
    Check (Float'Adjacent (1.0, 2.0) > 1.0);
    Check (Float'Adjacent (1.0, 1.0 + 2.0 ** (-100)) > 1.0);
    Check (Float'Adjacent (0.1, 0.1) - 0.1 = 0.0);
    Check (Converted = -1.0);
    Check (Long_Float'Copy_Sign (1.0, Long_Float (-0.0)) = -1.0);
    Check (Long_Float'Copy_Sign (1.0, (-0.0) ** 3) = -1.0);
    Check (Long_Float'Leading_Part (3.75, 2 ** 64) = 3.75);
    Check (Long_Float'Copy_Sign (1.0, -0.0) = -1.0);
    Check (Long_Float'Copy_Sign (1.0, Long_Float'Ceiling (-0.25)) = -1.0);
    Check (Long_Float'Copy_Sign (1.0, Long_Float'Floor (-0.0)) = -1.0);
    Check_Float;
    Check_Long;
    Check_Derived;
    Ada.Text_IO.Put_Line ("floating attributes");
end FloatAttributes;
