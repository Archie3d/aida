with Ada.Text_IO; use Ada.Text_IO;
procedure FixedOperators is
    type A is delta 0.125 range -10.0 .. 10.0;
    type B is delta 0.25 range -10.0 .. 10.0;
    function "*" (Left, Right : A) return A is
    begin
        return Left + Right;
    end "*";
    X : A := 1.5;
    Y : A := 2.0;
    Product : B := B (X * Y);
    Standard_Product : B := Standard."*" (X, Y);
    User_Product : A := X * Y;
    Static_Result : constant Long_Float := Long_Float (A'(1.5) * B'(2.0));
    Float_Result : Long_Float := Long_Float (Standard."/" (Y, X));
begin
    if Product /= 3.0 or Standard_Product /= 3.0 or User_Product /= 3.5
        or Static_Result /= 3.0 or Float_Result < 1.33 or Float_Result > 1.34 then
        raise Program_Error;
    end if;
    Put_Line ("fixed universal results and explicit primitive selection");
end FixedOperators;
