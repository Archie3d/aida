with Controlled_Result_Model; use Controlled_Result_Model;
with Ada.Finalization;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledResults is
    function Forward (Depth : Integer) return Guard is
    begin
        if Depth = 0 then
            return Make (42);
        end if;
        return Forward (Depth - 1);
    end Forward;
    function Make_Pair return Pair is
    begin
        return (Left => Make (11), Right => Forward (2));
    end Make_Pair;
    function Make_Array (Count : Integer) return Guards is
        Local : Guards (3 .. Count + 2);
    begin
        for I in Local'Range loop
            Local (I).Data.Value := I;
        end loop;
        return Local;
    end Make_Array;
    subtype Two is Guards (1 .. 2);
    function Make_Two return Two is
        Local : Two;
    begin
        return Local;
    end Make_Two;
    function Make_Matrix (Count : Integer) return Matrix is
        Local : Matrix (2 .. Count + 1, 5 .. 6);
    begin
        Local (2, 5).Data.Value := 99;
        return Local;
    end Make_Matrix;
    type Factory is access function (Value : Integer) return Guard;
    Imported : Factory := Make'Access;
    Nested : Factory := Forward'Access;
    procedure Borrow (Object : Guard) is
    begin
        Check (Object.Data.Value = 42);
    end Borrow;
    Initializes, Adjusts, Finalizes : Integer := 0;
    type Counter is new Ada.Finalization.Controlled with record
        Value : Integer := 0;
    end record;
    overriding procedure Initialize (Object : in out Counter) is
    begin
        Initializes := Initializes + 1;
    end Initialize;
    overriding procedure Adjust (Object : in out Counter) is
    begin
        Adjusts := Adjusts + 1;
    end Adjust;
    overriding procedure Finalize (Object : in out Counter) is
    begin
        Finalizes := Finalizes + 1;
    end Finalize;
    function Aggregate_Result return Counter is
    begin
        return (Ada.Finalization.Controlled with Value => 99);
    end Aggregate_Result;
begin
    declare
        Object : Counter := Aggregate_Result;
    begin
        Check (Object.Value = 99 and Initializes = 0 and Adjusts = 1 and Finalizes = 1);
    end;
    Check (Finalizes = 2);
    for Iteration in 1 .. 20 loop
        declare
            A : Guard := Forward (3);
            B : Pair := Make_Pair;
            C : Guards := Make_Array (3);
            D : Two := Make_Two;
            E : Matrix := Make_Matrix (2);
        begin
            Check (Objects = 12 and Resources = 12);
            Check (A.Data.Value = 42 and B.Left.Data.Value = 11);
            Check (C'First = 3 and C'Last = 5 and C (5).Data.Value = 5);
            Check (E'First (1) = 2 and E'First (2) = 5 and E (2, 5).Data.Value = 99);
            A := Imported (42);
            Borrow (Nested (2));
            Check (Objects = 12 and Resources = 12);
        end;
        Check (Objects = 0 and Resources = 0);
    end loop;
    declare
        Empty : Guards := Make_Array (0);
    begin
        Check (Empty'Length = 0 and Objects = 0 and Resources = 0);
    end;
    Put_Line ("controlled results passed");
end ControlledResults;
