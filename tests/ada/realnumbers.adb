with Ada.Text_IO;
with Real_Number_Model; use Real_Number_Model;
procedure RealNumbers is
    procedure Run is
        Left_Product : constant := (-3) * (-0.5);
        Right_Product : constant := (-0.5) * (-3);
        Ratio : constant := (-3.0) / (-2);
        Expanded : constant := Standard."*" (Left => 3, Right => 0.5);
        Converted_Product : Long_Float := Long_Float (3 * 0.5);
        Nested : constant := (3.0 / 2) * (7 / 2);
        Exact_Third : constant := 1.0 / 3;
        Exact_One : constant := Exact_Third * 3;
        Cancellation : constant := ((Factor * 0.125) + 1.0) - Product;
        Reciprocal : constant := (2 * 1.0) ** (-3);
        Rounded : constant := Float (16777217.0);
        Difference : constant := Rounded - 16777216.0;
        From_Real : Long_Float := Converted_Real;
        From_Integer : Integer := Converted_Integer;
        From_Fixed : Long_Float := Converted_Fixed;
        type Other_Real is digits 6;
        Other : Other_Real := Quotient;
        Single : Float := Float (1.0 + 1.0 / (2 ** 24) + 1.0 / (2 ** 80));
        type Fraction_2 is delta 0.125 range -8.0 .. 8.0;
        Fixed_Value : Fraction_2 := Quotient;
        Duration_Value : Duration := Duration'Small * 1048576;
        Negative_Duration : Duration := -Duration'Small * 1048576;
        function Choose (Value : Integer) return Boolean is
        begin
            return False;
        end Choose;
        function Choose (Value : Long_Float) return Boolean is
        begin
            return Value = 1.5;
        end Choose;
    begin
        if Left_Product /= 1.5 or Right_Product /= 1.5 or Ratio /= 1.5
            or Expanded /= 1.5 or Converted_Product /= 1.5
            or Nested /= 4.5 or Exact_One /= 1.0 or Cancellation /= 1.0
            or Reciprocal /= 0.125 or Difference /= 0.0
            or From_Real /= 1.25 or From_Integer /= 6 or From_Fixed /= 1.375
            or Other /= 0.125 or Single /= Float'Succ (1.0)
            or Fixed_Value /= 0.125 or not Choose (Left_Product)
            or Duration_Value /= 0.0009765625 or Negative_Duration /= -Duration_Value then
            raise Program_Error;
        end if;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run;
    exception
        when others => Ada.Text_IO.Put_Line ("real named-number worker failed");
    end Worker;
begin
    Run;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("real named-number arithmetic passed");
end RealNumbers;
