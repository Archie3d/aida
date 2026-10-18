procedure AttributeAuditErrors is
    type Color is (Red, Green);
    type Other is (Red, Green);
    C : Color := Red;
    O : Other := Red;
    I : Integer := 1;
    R : Float := 1.0;
    type Vector is array (Integer range <>) of Integer;
    type Matrix is array (Integer range <>, Integer range <>) of Integer;
    A : Matrix (1 .. 2, 3 .. 4);
    Bad_Pos : Integer := Color'Pos (O);
    Bad_Val : Color := Color'Val (R);
    Bad_Succ : Color := Color'Succ (O);
    Bad_Object : Integer := I'Pos (I);
    Bad_First : Integer := I'First;
    Bad_Real : Integer := Float'Pos (R);
    Bad_Array : Vector (1 .. 2) := Vector'Succ (A);
    Bad_Arity : Integer := Integer'First (1);
    Missing : Integer := Integer'Val;
    Extra : Integer := Integer'Pred (1, 2);
    Bad_Length : Integer := Integer'Length;
    Unbounded : Integer := Vector'First;
    Dimension_Zero : Integer := A'First (0);
    Dimension_Large : Integer := A'Last (3);
    Dimension_Real : Integer := A'First (1.0);
    Dimension_Dynamic : Integer := A'First (I);
    Dimension_Enum : Integer := A'First (C);
    Dimension_Extra : Integer := A'Last (1, 2);
    package Hidden is
        type Secret is private;
    private
        type Secret is range 1 .. 5;
    end Hidden;
    Private_First : Integer := Hidden.Secret'First;
    Private_Pos : Integer := Hidden.Secret'Pos (1);
    Oversized_Pred : Integer := Integer'Pred (2147483648);
    Undersized_Succ : Integer := Integer'Succ (-2147483649);
    Oversized_Pos : Long_Integer := Integer'Pos (2147483648);
    Oversized_Real : Float := Float'Pred (1.0E100);
begin
    for J in I'Range loop
        null;
    end loop;
end AttributeAuditErrors;
