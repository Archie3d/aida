with Ada.Finalization;
with Ada.Unchecked_Deallocation;
with Controlled_Result_Model; use Controlled_Result_Model;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledAllocations is
    procedure Run is
        type Guard_Link is access Guard;
        procedure Free is new Ada.Unchecked_Deallocation (Guard, Guard_Link);
        function Create return Guard_Link is
        begin
            return new Guard'(Make (42));
        end Create;
        A : Guard_Link := new Guard;
        B : Guard_Link := Create;
        Alias : Guard_Link := A;
        type Pair_Link is access Pair;
        procedure Free_Pair is new Ada.Unchecked_Deallocation (Pair, Pair_Link);
        P : Pair_Link := new Pair'(Left => A.all, Right => B.all);
        type Three is array (1 .. 3) of Guard;
        type Array_Link is access Three;
        procedure Free_Array is new Ada.Unchecked_Deallocation (Three, Array_Link);
        Items : Array_Link := new Three;
    begin
        Check (Objects = 7 and Resources = 5);
        Check (B.Data.Value = 42);
        A.all := B.all;
        Check (Alias.Data.Value = 42 and Objects = 7 and Resources = 5);
        Free (A);
        Check (A = null and Objects = 6);
        Free (A);
        Free_Pair (P);
        Free_Array (Items);
        Check (Objects = 1 and Resources = 1);
        -- B is left to the access type's collection, even after losing its name.
        B := null;
    end Run;
    procedure Recursive (Depth : Integer) is
        type Local_Link is access Guard;
        function Create return Local_Link is
        begin
            return new Guard;
        end Create;
        Object : Local_Link := Create;
    begin
        if Depth > 0 then
            Recursive (Depth - 1);
        end if;
    end Recursive;
    Limited_Live : Integer := 0;
    type Limited_Guard is new Ada.Finalization.Limited_Controlled with null record;
    overriding procedure Initialize (Object : in out Limited_Guard) is
    begin
        Limited_Live := Limited_Live + 1;
    end Initialize;
    overriding procedure Finalize (Object : in out Limited_Guard) is
    begin
        Limited_Live := Limited_Live - 1;
    end Finalize;
begin
    for Iteration in 1 .. 20 loop
        Run;
        Recursive (4);
        Check (Objects = 0 and Resources = 0);
        declare
            type Local_Link is access Guard;
            subtype Alias_Link is Local_Link;
            type Derived_Link is new Local_Link;
            Object : Alias_Link := new Guard;
            Other : Derived_Link := new Guard;
        begin
            Check (Objects = 2 and Resources = 2);
            Object := null;
        end;
        Check (Objects = 0 and Resources = 0);
    end loop;
    declare
        type Limited_Link is access Limited_Guard;
        procedure Free is new Ada.Unchecked_Deallocation (Limited_Guard, Limited_Link);
        A, B : Limited_Link := new Limited_Guard;
    begin
        Check (Limited_Live = 2);
        Free (A);
        Check (A = null and Limited_Live = 1);
    end;
    Check (Limited_Live = 0);
    declare
        type Node;
        type Node_Link is access Node;
        type Node is record
            Next : Node_Link := null;
            Item : Guard;
        end record;
        Root : Node_Link := new Node;
    begin
        Root.Next := new Node;
        Check (Objects = 2 and Resources = 2);
    end;
    Check (Objects = 0 and Resources = 0);
    Put_Line ("controlled allocations passed");
end ControlledAllocations;
