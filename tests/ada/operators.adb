with Ada.Text_IO; use Ada.Text_IO;
procedure Operators is
    Calls : Integer := 0;
    package Vectors is
        type Vector is record
            X, Y : Integer;
        end record;
        function "+" (Left, Right : Vector) return Vector;
        function "-" (Value : Vector) return Vector;
        function "*" (Left : Vector; Right : Integer) return Vector;
        function "=" (Left, Right : Vector) return Boolean;
        function "<" (Left, Right : Vector) return Boolean;
        function "ABS" (Value : Vector) return Integer;
        function "&" (Left, Right : Vector) return Vector;
        function "not" (Value : Vector) return Vector;
        function "and" (Left, Right : Vector) return Vector;
        function "or" (Left, Right : Vector) return Vector;
        function "xor" (Left, Right : Vector) return Vector;
    end Vectors;
    package body Vectors is
        function "+" (Left, Right : Vector) return Vector is
        begin
            Calls := Calls + 1;
            return (Left.X + Right.X, Left.Y + Right.Y);
        end "+";
        function "-" (Value : Vector) return Vector is
        begin
            return (-Value.X, -Value.Y);
        end "-";
        function "*" (Left : Vector; Right : Integer) return Vector is
        begin
            return (Left.X * Right, Left.Y * Right);
        end "*";
        function "=" (Left, Right : Vector) return Boolean is
        begin
            -- Deliberately differs from predefined record equality.
            return Left.X = Right.X;
        end "=";
        function "<" (Left, Right : Vector) return Boolean is
        begin
            return Left.X < Right.X;
        end "<";
        function "abs" (Value : Vector) return Integer is
        begin
            return abs Value.X + abs Value.Y;
        end "Abs";
        function "&" (Left, Right : Vector) return Vector is
        begin
            return (Left.X, Right.Y);
        end "&";
        function "not" (Value : Vector) return Vector is
        begin
            return (Value.Y, Value.X);
        end "not";
        function "and" (Left, Right : Vector) return Vector is
        begin
            return (Left.X * Right.X, Left.Y * Right.Y);
        end "and";
        function "or" (Left, Right : Vector) return Vector is
        begin
            return (Left.X, Right.Y);
        end "or";
        function "xor" (Left, Right : Vector) return Vector is
        begin
            return (Left.Y, Right.X);
        end "xor";
    end Vectors;
    use Vectors;
    A : Vector := (2, 3);
    B : Vector := (2, 9);
    C : Vector;
    function Pick return Vector is
    begin
        return A;
    end Pick;
    function Pick return Integer is
    begin
        return 4;
    end Pick;
    procedure Show (Value : Vector) is
    begin
        Put_Line (Integer'Image (Value.X) & Integer'Image (Value.Y));
    end Show;
    function Touch return Boolean is
    begin
        Calls := Calls + 100;
        return True;
    end Touch;
begin
    C := A + B;
    Show (C);
    Show (Vectors."+" (A, B));
    Show ("+" (Left => A, Right => B));
    Show (-(A + B) * 2);
    Show (Pick + A);
    Show (A * Pick);
    Put_Line (Integer'Image (abs A));
    Put_Line (Integer'Image (Vectors."ABS" (A)));
    Show (A & B);
    Show (not A);
    Show (A and B);
    Show (A or B);
    Show (A xor B);
    if A = B and not (A /= B) and not Vectors."/=" (A, B) then
        Put_Line ("equality");
    end if;
    if A < C then
        Put_Line ("ordering");
    end if;
    if False and then Touch then
        raise Program_Error;
    end if;
    if True or else Touch then
        null;
    end if;
    Put_Line (Integer'Image (Calls));
    Put_Line (Integer'Image ("+" (2, 3)));
    -- A local homograph replaces its predefined numeric operation.
    declare
        function "+" (Left, Right : Integer) return Integer is
        begin
            return Left - (-Right) - 1;
        end "+";
        X : Integer := 2;
        Named_Number : constant := 2 + 3;
    begin
        Put_Line (Integer'Image (Named_Number));
        Put_Line (Integer'Image (X + 3));
        Put_Line (Integer'Image ("+" (X, 3)));
    end;
end Operators;
