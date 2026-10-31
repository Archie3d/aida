with Ada.Text_IO;
procedure Generic_Dynamic_Objects is
    type Vector is array (Integer range <>) of Integer;
    type Matrix is array (Integer range <>, Integer range <>) of Integer;
    generic
        Initial : Vector;
        Target : in out Vector;
        Backup : Vector := Initial;
    package Vectors is
        function First return Integer;
        function Last return Integer;
        function Read return Vector;
        procedure Restore;
    end Vectors;
    package body Vectors is
        function First return Integer is
        begin
            return Initial'First;
        end First;
        function Last return Integer is
        begin
            return Initial'Last;
        end Last;
        function Read return Vector is
        begin
            return Backup;
        end Read;
        procedure Restore is
        begin
            Target := Backup;
        end Restore;
    end Vectors;
    generic
        Initial : Matrix;
        Target : in out Matrix;
    procedure Restore_Matrix;
    procedure Restore_Matrix is
    begin
        Target := Initial;
        Target (Target'First (1), Target'Last (2)) := 99;
    end Restore_Matrix;
    procedure Run (Size, Depth : Integer) is
        subtype Local_Vector is Vector (1 .. Size);
        generic
            Initial : Local_Vector;
            Target : in out Vector;
        package Nested is
            package Forwarded is new Vectors (Initial, Target);
        end Nested;
        Data : Vector (4 .. Size + 3) := (others => Size);
        Target : Vector (-2 .. Size - 3) := (others => 0);
        package First is new Vectors (Data, Target);
        package Forwarding is new Nested (Data, Target);
        package Slice_View is new Vectors (Data (5 .. Size + 2), Target (-1 .. Size - 4));
        function Make return Vector is
            Result : Vector (8 .. Size + 7) := (others => 12);
        begin
            return Result;
        end Make;
        package Temporary is new Vectors (Make, Target);
        M : Matrix (2 .. Size + 1, 5 .. 6) := (others => (others => Size));
        N : Matrix (-1 .. Size - 2, 8 .. 9) := (others => (others => 0));
        procedure Restore is new Restore_Matrix (M, N);
    begin
        Data := (others => 0);
        M := (others => (others => 0));
        if Depth > 0 then
            Run (Size + 1, Depth - 1);
        end if;
        Forwarding.Forwarded.Restore;
        if Forwarding.Forwarded.First /= 1 or Forwarding.Forwarded.Last /= Size
            or Target (-2) /= Size then
            raise Program_Error;
        end if;
        First.Restore;
        if First.First /= 4 or First.Last /= Size + 3 or Target (-2) /= Size then
            raise Program_Error;
        end if;
        Target := (others => 0);
        Slice_View.Restore;
        if Target (-2) /= 0 or Target (-1) /= Size or Target (Size - 3) /= 0 then
            raise Program_Error;
        end if;
        Temporary.Restore;
        if Target (-2) /= 12 or Temporary.First /= 8 or Temporary.Last /= Size + 7 then
            raise Program_Error;
        end if;
        Restore;
        if N (-1, 9) /= 99 or N (Size - 2, 8) /= Size then
            raise Program_Error;
        end if;
        declare
            Returned : Vector := First.Read;
        begin
            if Returned'First /= 4 or Returned'Length /= Size or Returned (4) /= Size then
                raise Program_Error;
            end if;
        end;
    end Run;
    Empty : Vector (5 .. 4);
    package Null_Array is new Vectors (Empty, Empty);
begin
    Run (4, 2);
    Run (5, 0);
    Null_Array.Restore;
    if Null_Array.First /= 5 or Null_Array.Last /= 4 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("generic runtime arrays: bounds, slices, temporaries, matrices, recursion");
end Generic_Dynamic_Objects;
