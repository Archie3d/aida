with Ada.Text_IO; use Ada.Text_IO;
procedure AggregateAudit is
    Calls : Integer := 0;
    function Next return Integer is
    begin
        Calls := Calls + 1;
        return Calls;
    end Next;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    type Vector is array (Positive range <>) of Integer;
    A : Vector (5 .. 7) := (1 => 11, 2 .. 3 => 22);
    B : Vector (1 .. 4) := (2 | 4 => 8, others => Next);
    Empty : Vector (8 .. 3) := (others => Next);
    Inferred : Vector := (9 .. 10 => 7);
    type Pair is record
        Left, Right : Integer := Next;
    end record;
    type Parent is tagged record
        Left, Right : Integer := Next;
    end record;
    type Child is new Parent with record
        Extra : Integer := Next;
    end record;
    type Matrix is array (Positive range <>, Positive range <>) of Integer;
    Grid : Matrix (1 .. 2, 3 .. 4) := (5 .. 6 => (7 .. 8 => 6));
    Null_Grid : Matrix (1 .. 0, 1 .. 2) := (others => (others => Next));
begin
    Check (Calls = 2 and A (5) = 11 and A (7) = 22);
    Check (B (2) = 8 and B (4) = 8 and B (1) + B (3) = 3);
    Check (Empty'Length = 0 and Inferred'First = 9 and Inferred'Last = 10);
    Check (Grid (2, 4) = 6 and Null_Grid'Length (1) = 0);
    declare
        Explicit : Pair := (Left | Right => Next);
    begin
        Check (Calls = 4 and Explicit.Left + Explicit.Right = 7);
    end;
    declare
        Defaulted : Child := (Parent with Extra => 99);
    begin
        Check (Calls = 6 and Defaulted.Left + Defaulted.Right = 11);
        Check (Defaulted.Extra = 99);
    end;
    A := (1, 2, 3);
    A := (A (7), A (6), A (5));
    Check (A (5) = 3 and A (7) = 1);
    declare
        R : Pair := (1, 2);
    begin
        R := (R.Right, R.Left);
        Check (R.Left = 2 and R.Right = 1);
    end;
    Put_Line ("aggregate choices, sliding, defaults, null shapes and simultaneous assignment passed");
end AggregateAudit;
