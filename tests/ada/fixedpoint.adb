with Ada.Text_IO;
procedure Fixedpoint is
    Step : constant := 0.125;
    type Fixed is delta Step range -100.0 .. 100.0;
    type Fine is delta 0.01 range -100.0 .. 100.0;
    subtype Positive_Fixed is Fixed range 0.125 .. 10.0;
    A : Fixed := 1.25;
    B : Fixed := 2.5;
    X : Fixed;
    I : Integer := 3;
    F : Float := 1.5;
    Small : constant Fixed := Fixed'Small;
    Rounded : constant Fixed := 0.0625;
    Sum : constant Fixed := 1.25 + 2.5;
    Exact : constant := 0.062499999999999999999999999999;
    Below : Fixed := Exact;
    function Add (Left, Right : Fixed) return Fixed is
    begin
        return Left + Right;
    end Add;
    procedure Increment (Value : in out Fixed) is
    begin
        Value := Value + 0.125;
    end Increment;
    type Values is array (1 .. 2) of Fixed;
    Data : Values := (A, B);
begin
    if Fixed'Small /= 0.125 or Fixed'Delta /= Step or Fine'Small /= 0.0078125
        or Small /= 0.125 or Rounded /= 0.125 or Below /= 0.0 then
        raise Program_Error;
    end if;
    if Add (A, B) /= Sum or A - B /= -1.25 or abs (-A) /= A then
        raise Program_Error;
    end if;
    X := A * B;
    if X /= 3.125 or X /= Fixed (1.25 * 2.5) then
        raise Program_Error;
    end if;
    X := B / A;
    if X /= 2.0 then raise Program_Error; end if;
    X := A * I;
    if X /= 3.75 or I * A /= X then raise Program_Error; end if;
    X := A / I;
    if X /= 0.375 then raise Program_Error; end if;
    X := Fixed (Fine (0.0625));
    if X /= 0.125 or Fixed (F) /= 1.5 or Float (B) /= 2.5
        or Integer (B) /= 3 or Integer (-B) /= -3 or Fixed (I) /= 3.0 then
        raise Program_Error;
    end if;
    Increment (Data (1));
    if Data (1) /= 1.375 or Positive_Fixed'First /= 0.125 or Fixed'Last /= 100.0 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("fixed point: exact constants, arithmetic, conversions, and storage");
end Fixedpoint;
