with Ada.Text_IO; use Ada.Text_IO;
with Ada.Unchecked_Deallocation;
with Controlled_Result_Model; use Controlled_Result_Model;
procedure ControlledDynamic is
    type Root is tagged null record;
    type Child is new Root with record
        First, Second : Guard;
    end record;
    function Make return Root'Class is
        Object : Child;
    begin
        Object.First.Data.Value := 42;
        return Object;
    end Make;
    function Forward return Root'Class is
    begin
        return Make;
    end Forward;
    type Factory is access function return Root'Class;
    Callback : Factory := Forward'Access;
    procedure Run is
        type Link is access Root'Class;
        procedure Free is new Ada.Unchecked_Deallocation (Root'Class, Link);
        A : Root'Class := Callback.all;
        B : Root'Class := A;
        Heap : Link := new Root'Class'(B);
    begin
        Check (Objects = 6 and Resources = 2);
        Check (Child (Heap.all).First.Data.Value = 42);
        A := B;
        A := A;
        Check (Objects = 6 and Resources = 2);
        Free (Heap);
        Check (Objects = 4 and Resources = 2 and Heap = null);
        Fail_Adjust := 2;
        begin
            Heap := new Root'Class'(A);
            raise Constraint_Error;
        exception
            when Program_Error => Check (Objects = 4 and Resources = 2 and Heap = null);
        end;
        Fail_Adjust := 1;
        begin
            declare
                Failed : Root'Class := B;
            begin
                raise Constraint_Error;
            end;
        exception
            when Program_Error => Check (Objects = 4 and Resources = 2);
        end;
        Heap := new Root'Class'(Forward);
        Check (Objects = 6 and Resources = 4);
    end Run;
begin
    Run;
    Check (Objects = 0 and Resources = 0);
    Put_Line ("dynamic controlled ownership passed");
end ControlledDynamic;
