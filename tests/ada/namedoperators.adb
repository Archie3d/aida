with Ada.Text_IO; use Ada.Text_IO;
procedure NamedOperators is
    A : Integer := -13;
    B : Integer := 5;
    Wide : Long_Integer := 13;
    Calls : Integer := 0;
    function Once (Value : Integer) return Integer is
    begin
        Calls := Calls + 1;
        return Value;
    end Once;
    procedure Check (Value : Boolean) is
    begin
        if not Value then
            raise Program_Error;
        end if;
    end Check;
    function Pick (Value : Integer) return Integer is
    begin
        return Value;
    end Pick;
    function Pick (Value : Boolean) return Integer is
    begin
        return 999;
    end Pick;
    function Number return Integer is
    begin
        return 13;
    end Number;
    function Number return Long_Integer is
    begin
        return 23;
    end Number;
    type Word is mod 256;
    M : Word := 250;
    type Fixed is delta 0.125 range -100.0 .. 100.0;
    F : Fixed := 3.0;
    R : Float := 3.0;
    type Box is record
        Value : Integer;
    end record;
    function "+" (X, Y : Box) return Integer is
    begin
        return X.Value * 10 + Y.Value;
    end "+";
    function "-" (Right, Left : Box) return Integer is
    begin
        return Right.Value * 10 + Left.Value;
    end "-";
    function "abs" (Item : Box) return Integer is
    begin
        return Item.Value;
    end "abs";
    X : Box := (Value => 2);
    Y : Box := (Value => 7);
    -- Formal names can select among otherwise competing user profiles.
    function "*" (First : Integer; Second : Long_Integer) return Integer is
    begin
        return 17;
    end "*";
    function "*" (This : Long_Integer; That : Integer) return Integer is
    begin
        return 29;
    end "*";
    generic
        type Scalar is private;
        with function "rem" (Left, Right : Scalar) return Scalar is <>;
    package Calculations is
        function Apply (A, B : Scalar) return Scalar;
    end Calculations;
    package body Calculations is
        function Apply (A, B : Scalar) return Scalar is
        begin
            return "rem" (Right => B, Left => A);
        end Apply;
    end Calculations;
    package Integer_Calculations is new Calculations (Integer);
    function Calculate_Remainder (Dividend, Divisor : Integer) return Integer is
    begin
        return Dividend rem Divisor;
    end Calculate_Remainder;
    package Custom_Calculations is new Calculations (Integer, Calculate_Remainder);
    Zero : Integer := 0;
    Target : Integer := 123;
    Caught : Boolean := False;
    Static_Rem : constant := "REM" (Right => 5, Left => 13);
    subtype Span is Integer range 0 .. Static_Rem;
    type Elements is array (Span) of Integer;
    Data : Elements := (others => 0);
begin
    Check ("REM" (LEFT => A, RIGHT => B) = -3);
    Check ("mOd" (rIgHt => B, lEfT => A) = 2);
    Check ("rem" (A, Right => B) = -3);
    Check ("rem" (Right => 5, Left => Wide) = 3);
    Check ("/" (Right => B, Left => 13) = 2);
    Check ("-" (Right => B, Left => 13) = 8);
    Check ("**" (Right => 3, Left => 2) = 8);
    Check ("-" (Right => B) = -5);
    Check ("ABS" (Right => A) = 13);
    Check ("+" (Right => B) = 5);
    Check ("NOT" (Right => False));
    Check ("AND" (Right => True, Left => True));
    Check ("<" (Right => B, Left => A));
    Check ("=" (Right => B, Left => 5));
    Check ("&" (Right => "b", Left => "a") = "ab");
    Check ("+" (Right => Word'(10), Left => M) = 4);
    Check ("+" (Right => 2.0, Left => R) = 5.0);
    Check ("*" (Right => 2, Left => F) = 6.0);
    Check (Standard."*" (Right => 2, Left => F) = 6.0);
    Check (Data'Length = 4);
    Check (Pick ("rem" (Right => B, Left => Number)) = 3);
    Check ("+" (Y => Y, X => X) = 27);
    Check ("-" (Right => Y, Left => X) = 72);
    Check ("abs" (Item => X) = 2);
    Check ("*" (Second => 2, First => 1) = 17);
    Check ("*" (That => 2, This => 1) = 29);
    Check ("rem" (Right => Once (5), Left => Once (13)) = 3);
    Check (Calls = 2);
    Check (Integer_Calculations.Apply (A, B) = -3);
    Check (Custom_Calculations.Apply (A, B) = -3);
    begin
        Target := "rem" (Right => Zero, Left => A);
    exception
        when Constraint_Error => Caught := True;
    end;
    Check (Caught and then Target = 123);
    -- An explicit homograph keeps its own formal names and implementation.
    declare
        function "rem" (Dividend, Divisor : Integer) return Integer is
        begin
            return 42;
        end "rem";
    begin
        Check ("rem" (Divisor => B, Dividend => A) = 42);
    end;
    Put_Line ("named operator associations passed");
end NamedOperators;
