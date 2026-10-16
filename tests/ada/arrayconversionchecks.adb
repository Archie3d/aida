with Ada.Text_IO; use Ada.Text_IO;
procedure ArrayConversionChecks is
    type Vector is array (Integer range <>) of Integer;
    subtype Small_Index is Integer range 1 .. 2;
    type Small is array (Small_Index range <>) of Integer;
    type Positive_Vector is array (Positive range <>) of Integer;
    subtype Negative is Vector (-2 .. -1);
    subtype Pair is Small (1 .. 2);
    A : Vector (3 .. 5) := (1, 2, 3);
    P : Positive_Vector (1 .. 2) := (4, 5);
    Entered : Boolean := False;
    procedure Touch (X : in out Small) is
    begin
        Entered := True;
        X (X'First) := 99;
    end Touch;
    procedure Fail (X : in out Small) is
    begin
        X (X'First) := 99;
        raise Constraint_Error;
    end Fail;
    procedure Fill (X : out Vector) is
    begin
        X := (7, 8);
    end Fill;
    procedure Copy_Back (X : in out Positive_Vector) is
    begin
        Fill (Negative (X));
    end Copy_Back;
    type Matrix is array (Integer range <>, Integer range <>) of Integer;
    type Small_Matrix is array (Small_Index range <>, Small_Index range <>) of Integer;
    M : Matrix (1 .. 0, 5 .. 6) := (others => (others => 0));
    subtype Square is Small_Matrix (1 .. 2, 1 .. 2);
    type Modular_Index is mod 8;
    type Modular_Vector is array (Modular_Index range <>) of Integer;
    Null_Source : Vector (-1 .. -2) := (others => 0);
    procedure Fixed (X : in out Pair) is
    begin
        Entered := True;
    end Fixed;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
begin
    begin
        Touch (Small (A));
        raise Program_Error;
    exception
        when Constraint_Error => Check (not Entered);
    end;
    begin
        Touch (Pair (A));
        raise Program_Error;
    exception
        when Constraint_Error => Check (not Entered);
    end;
    begin
        Fixed (Small (A (3 .. 5)));
        raise Program_Error;
    exception
        when Constraint_Error => Check (not Entered);
    end;
    begin
        declare
            Invalid : Modular_Vector := Modular_Vector (Null_Source);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => null;
    end;
    begin
        Fail (Small (P));
        raise Program_Error;
    exception
        when Constraint_Error => Check (P (1) = 4);
    end;
    begin
        Copy_Back (P);
        raise Program_Error;
    exception
        when Constraint_Error => Check (P (1) = 4 and P (2) = 5);
    end;
    begin
        declare
            Invalid : Small_Matrix := Small_Matrix (M);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => null;
    end;
    begin
        declare
            Invalid : Square := Square (M);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => null;
    end;
    Put_Line ("array conversion bounds, shape, propagation and copy-back checks passed");
end ArrayConversionChecks;
