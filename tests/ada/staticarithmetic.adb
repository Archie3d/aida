with Ada.Text_IO; use Ada.Text_IO;
with Static_Numbers;

procedure StaticArithmetic is
    Huge : constant := Static_Numbers.Huge;
    Real_Huge : constant := Static_Numbers.Real_Huge;
    Minimum : Long_Integer := -9223372036854775808;
    Based_Minimum : Long_Integer := -16#8000_0000_0000_0000#;
    Cancellation : Integer := (Huge + 7) - Huge;
    Product : Integer := (2 ** 70 * 2 ** 30) / Huge;
    Real_Cancellation : Float := (Real_Huge + 1.0) - Real_Huge;
    Rounded : constant Float := 16777217.0;
    Rounded_Difference : Float := Rounded - 16777216.0;
    Above_Tie : Float := 1.0 + 2.0 ** (-24) + 2.0 ** (-80);
    Below_Tie : Float := 1.0 + 2.0 ** (-24) - 2.0 ** (-80);
    At_Tie : Float := 1.0 + 2.0 ** (-24);
    Negative_Tie : Float := -1.0 - 2.0 ** (-24) - 2.0 ** (-80);
    Double_Tie : Long_Float := 1.0 + 2.0 ** (-53) + 2.0 ** (-100);
    Skipped_And : Boolean := False and then 1 / 0 = 0;
    Skipped_Or : Boolean := True or else 2 ** (-1) = 0;
    Skipped_Attribute : Boolean := True or else Integer'Succ (Integer'Last) = 0;
    Skipped_Real : Boolean := False and then -(1.0 / 0.0) = 0.0;
    Skipped_Check : Boolean := False and then Positive'(0) = 0;
    Skipped_Prefix : Boolean := True or else Long_Integer'Pos (Huge) = 0;
    Large_Real : Float := Float (Huge);
    Positive : Integer := (-7) mod 3;
    Negative : Integer := 7 mod (-3);
    Remainder : Integer := (-7) rem 3;
    Quotient : Integer := (-7) / 3;
    type Counter is mod 256;
    Wrapped : Counter := 255 + 2;
    function Dynamic (Value : Long_Integer) return Long_Integer is
    begin
        return Value + 1;
    end Dynamic;
begin
    if Minimum /= Long_Integer'First or Based_Minimum /= Minimum
        or Cancellation /= 7 or Product /= 1 or Real_Cancellation /= 1.0
        or Rounded_Difference /= 0.0 or Skipped_And or not Skipped_Or
        or not Skipped_Attribute or Skipped_Real or Skipped_Check or not Skipped_Prefix
        or Large_Real /= 2.0 ** 100
        or Above_Tie /= Float'Succ (1.0) or Below_Tie /= 1.0 or At_Tie /= 1.0
        or Negative_Tie /= -Float'Succ (1.0) or Double_Tie /= Long_Float'Succ (1.0)
        or Positive /= 2 or Negative /= -2 or Remainder /= -1
        or Quotient /= -2 or Wrapped /= 1 then
        raise Program_Error;
    end if;
    begin
        Minimum := Dynamic (Long_Integer'Last);
        raise Program_Error;
    exception
        when Constraint_Error => Put_Line ("runtime overflow preserved");
    end;
    Put_Line ("static arithmetic");
end StaticArithmetic;
