with Ada.Finalization;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledArrays is
    Live : Integer := 0;
    Adjustments : Integer := 0;
    Temporary_Finalizes : Integer := 0;
    type Guard is new Ada.Finalization.Controlled with record
        Owned : Boolean := False;
    end record;
    overriding procedure Initialize (Object : in out Guard) is
    begin
        Live := Live + 1;
        Object.Owned := True;
    end Initialize;
    overriding procedure Adjust (Object : in out Guard) is
    begin
        Adjustments := Adjustments + 1;
        Live := Live + 1;
    end Adjust;
    overriding procedure Finalize (Object : in out Guard) is
    begin
        if Object.Owned then
            Live := Live - 1;
        else
            Temporary_Finalizes := Temporary_Finalizes + 1;
        end if;
    end Finalize;
    Limited_Finalizes : Integer := 0;
    type Limited_Guard is new Ada.Finalization.Limited_Controlled with null record;
    overriding procedure Finalize (Object : in out Limited_Guard) is
    begin
        Limited_Finalizes := Limited_Finalizes + 1;
    end Finalize;
    type Limited_Array is array (Positive range <>) of Limited_Guard;
    type Guards is array (Positive range <>) of Guard;
    type Grid is array (Positive range <>, Positive range <>) of Guard;
    procedure Assign (Target : in out Grid; Source : Grid) is
    begin
        Target := Source;
    end Assign;
    function Size return Integer is
    begin
        return 2;
    end Size;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    -- Repeated evaluation of the same aggregate AST must clean each temporary.
    function Observe (Item : Guard) return Integer is
    begin
        return 42;
    end Observe;
    type Element is record
        Item : Guard;
        Number : Integer := Observe (Guard'(Ada.Finalization.Controlled with Owned => False));
    end record;
    type Elements is array (Positive range <>) of Element;
begin
    declare
        N : Integer := Size;
        Source : Guard;
        A : Grid (1 .. N, 1 .. N) := (others => (others => Source));
        B : Grid (5 .. 4 + N, 5 .. 4 + N) := A;
        Empty : Grid (2 .. 1, 1 .. N);
    begin
        Check (Live = 9);
        Assign (B, A);
        Check (Live = 9);
        Empty := Empty;
    end;
    Check (Live = 0);
    Put_Line ("multidimensional arrays");
    declare
        Items : Elements (1 .. Size + 1);
    begin
        Check (Live = 3 and Temporary_Finalizes = 3 and Items(3).Number = 42);
    end;
    Check (Live = 0);
    Put_Line ("component expression temporaries");
    declare
        Before : Integer := Adjustments;
        Source : Guard;
        Items : Guards := (1 .. Size => Source);
        Qualified : Guards (1 .. Size) := Guards'(1 .. Size => Source);
    begin
        Check (Live = 5 and Adjustments = Before + 4);
    end;
    Check (Live = 0);
    Put_Line ("inferred and qualified aggregates");
    declare
        Items : Limited_Array :=
            (1 .. Size => (Ada.Finalization.Limited_Controlled with null record));
    begin
        Check (Limited_Finalizes = 0);
    end;
    Check (Limited_Finalizes = 2);
    Put_Line ("limited aggregate construction");
end ControlledArrays;
