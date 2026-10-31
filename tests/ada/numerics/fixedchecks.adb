with Ada.Text_IO;
procedure Fixedchecks is
    type Fixed is delta 0.125 range -10.0 .. 10.0;
    subtype Positive_Fixed is Fixed range 0.125 .. 5.0;
    type Wide is delta 1.0 range -9223372036854775808.0 .. 9223372036854775807.0;
    A : Fixed := 2.5;
    Zero : Fixed := 0.0;
    Large : Float := 100.0;
    X : Fixed;
    High : Wide := Wide'Last;
    Low : Wide := -9223372036854775808.0;
    W : Wide;
    Caught : Integer := 0;
    Value : Positive_Fixed := 1.0;
    procedure Set_Zero (Item : out Fixed) is
    begin
        Item := 0.0;
    end Set_Zero;
    function Too_Large return Fixed is
    begin
        return A * 5;
    end Too_Large;
begin
    begin
        X := A / Zero;
        raise Program_Error;
    exception when Constraint_Error => Caught := Caught + 1;
    end;
    begin
        X := Fixed (Large);
        raise Program_Error;
    exception when Constraint_Error => Caught := Caught + 1;
    end;
    begin
        W := High + 1.0;
        raise Program_Error;
    exception when Constraint_Error => Caught := Caught + 1;
    end;
    begin
        W := -Low;
        raise Program_Error;
    exception when Constraint_Error => Caught := Caught + 1;
    end;
    begin
        W := High * High;
        raise Program_Error;
    exception when Constraint_Error => Caught := Caught + 1;
    end;
    begin
        Set_Zero (Value);
        raise Program_Error;
    exception when Constraint_Error => Caught := Caught + 1;
    end;
    begin
        X := Too_Large;
        raise Program_Error;
    exception when Constraint_Error => Caught := Caught + 1;
    end;
    begin
        X := Fixed'(A * 5);
        raise Program_Error;
    exception when Constraint_Error => Caught := Caught + 1;
    end;
    if Caught /= 8 or Value /= 1.0 or Low /= Wide'First then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("fixed-point overflow, zero division, range checks, and copy-back");
end Fixedchecks;
