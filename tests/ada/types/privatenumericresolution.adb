with Ada.Text_IO;
procedure PrivateNumericResolution is
    package Numbers is
        type Whole is private;
        type Real_Number is private;
        type Fixed_Number is private;
        function Pick (Value : Integer) return Integer;
        function Pick (Value : Whole) return Integer;
        function Pick_Real (Value : Float) return Integer;
        function Pick_Real (Value : Real_Number) return Integer;
        function Pick_Fixed (Value : Float) return Integer;
        function Pick_Fixed (Value : Fixed_Number) return Integer;
        function Make return Whole;
    private
        type Whole is new Integer;
        type Real_Number is digits 6;
        type Fixed_Number is delta 0.25 range -10.0 .. 10.0;
        Zero : Whole := 0;
        Real_Zero : Real_Number := 0.0;
        Fixed_Zero : Fixed_Number := 0.0;
    end Numbers;
    package body Numbers is
        function Pick (Value : Integer) return Integer is
        begin
            return 1;
        end Pick;
        function Pick (Value : Whole) return Integer is
        begin
            return 2;
        end Pick;
        function Pick_Real (Value : Float) return Integer is
        begin
            return 3;
        end Pick_Real;
        function Pick_Real (Value : Real_Number) return Integer is
        begin
            return 4;
        end Pick_Real;
        function Pick_Fixed (Value : Float) return Integer is
        begin
            return 5;
        end Pick_Fixed;
        function Pick_Fixed (Value : Fixed_Number) return Integer is
        begin
            return 6;
        end Pick_Fixed;
        function Make return Whole is
            Result : Whole := 3;
        begin
            return Result + Zero;
        end Make;
    end Numbers;
    Three : constant := 3;
    One : constant := 1.0;
begin
    if Numbers.Pick (3) /= 1 or else Numbers.Pick (Three) /= 1
        or else Numbers.Pick (1 + 2) /= 1
        or else Numbers.Pick_Real (1.0) /= 3
        or else Numbers.Pick_Real (One) /= 3
        or else Numbers.Pick_Fixed (1.0) /= 5
        or else Numbers.Pick_Fixed (One) /= 5
        or else Numbers.Pick (Numbers.Make) /= 2 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("private numeric resolution passed");
end PrivateNumericResolution;
