with Ada.Text_IO; use Ada.Text_IO;
procedure DerivedConstraints is
    type Number is range -100 .. 100;
    subtype Small is Number range 1 .. 3;
    type Count is new Small;
    V : Number := 9;
    function Read (X : Number) return Integer is
    begin
        return Integer (X);
    end Read;
    Limit : Integer := 3;
    subtype Dynamic is Integer range 1 .. Limit;
    type Dynamic_Child is new Dynamic;
    X : Dynamic_Child := 2;
    type Real_Number is digits 6 range -10.0 .. 10.0;
    subtype Small_Real is Real_Number range 1.0 .. 2.0;
    type Real_Child is new Small_Real;
    R : Real_Number := 3.0;
    type Fixed_Number is delta 0.25 range -10.0 .. 10.0;
    subtype Small_Fixed is Fixed_Number range 1.0 .. 2.0;
    type Fixed_Child is new Small_Fixed;
    F : Fixed_Number := 3.0;
    type Modular is mod 256;
    subtype Small_Modular is Modular range 1 .. 3;
    type Modular_Child is new Small_Modular;
    M : Modular := 4;
    type Vector is array (Integer range <>) of Integer;
    subtype Pair is Vector (3 .. 4);
    type Derived_Pair is new Pair;
    A : Vector (8 .. 9) := (11, 12);
    B : Derived_Pair := Derived_Pair (A);
    Long : Vector (1 .. 3) := (1, 2, 3);
begin
    Put_Line (Integer'Image (Integer (Count'Base (V))));
    Put_Line (Integer'Image (Derived_Pair'First));
    Put_Line (Integer'Image (B (4)));
    Limit := 1;
    Put_Line (Integer'Image (Dynamic_Child'Last));
    begin
        Put_Line (Integer'Image (Read (Number (Count (V)))));
    exception
        when Constraint_Error => Put_Line ("integer constraint");
    end;
    begin
        X := Dynamic_Child (V);
    exception
        when Constraint_Error => Put_Line ("dynamic constraint");
    end;
    begin
        Put_Line (Real_Number'Image (Real_Number (Real_Child (R))));
    exception
        when Constraint_Error => Put_Line ("real constraint");
    end;
    begin
        Put_Line (Fixed_Number'Image (Fixed_Number (Fixed_Child (F))));
    exception
        when Constraint_Error => Put_Line ("fixed constraint");
    end;
    begin
        Put_Line (Modular'Image (Modular (Modular_Child (M))));
    exception
        when Constraint_Error => Put_Line ("modular constraint");
    end;
    begin
        B := Derived_Pair (Long);
    exception
        when Constraint_Error => Put_Line ("array constraint");
    end;
end DerivedConstraints;
