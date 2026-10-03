with Ada.Finalization;
with Ada.Unchecked_Deallocation;
with Ada.Text_IO; use Ada.Text_IO;
with Controlled_Result_Model; use Controlled_Result_Model;
procedure ControlledWideAllocations is
    type Outer_Link is access Guard'Class;
    Escaped : Outer_Link;
    procedure Escape is
        type Local is new Guard with null record;
    begin
        Escaped := new Local;
        raise Constraint_Error;
    exception
        when Program_Error => Check (Escaped = null and Objects = 0 and Resources = 0);
    end Escape;
    procedure Exercise (Depth : Integer) is
        Finalized : Integer := 0;
        type Child is new Guard with record
            Extra : Guard;
        end record;
        overriding procedure Finalize (Object : in out Child) is
        begin
            Finalized := Finalized + 1;
            Controlled_Result_Model.Finalize (Guard (Object));
        end Finalize;
        overriding function Make (Value : Integer) return Child is
            Object : Child;
        begin
            Object.Data.Value := Value;
            return Object;
        end Make;
        type Link is access Guard'Class;
        procedure Free is new Ada.Unchecked_Deallocation (Guard'Class, Link);
        A : Link := new Child;
        B : Link := new Guard'(Make (42));
        Before : Integer := 0;
    begin
        Before := Objects;
        Check (A.all in Child and B.Data.Value = 42);
        if Depth > 0 then
            Exercise (Depth - 1);
            Check (Objects = Before);
        end if;
        Free (A);
        Check (A = null and Objects = Before - 2 and Finalized = 1);
        Free (A);
        -- B remains owned by this access type's collection.
    end Exercise;
    procedure Component_Only is
        type Root is tagged null record;
        type Child is new Root with record
            First, Second : Guard;
        end record;
        type Link is access Root'Class;
        procedure Free is new Ada.Unchecked_Deallocation (Root'Class, Link);
        A : Link := new Child;
        B : Link := new Child'(Child (A.all));
    begin
        Check (Objects = 4 and Resources = 2);
        Free (A);
        Check (Objects = 2 and Resources = 2);
    end Component_Only;
    procedure Limited_Allocation is
        Live : Integer := 0;
        type Guard is new Ada.Finalization.Limited_Controlled with null record;
        overriding procedure Initialize (Object : in out Guard) is
        begin
            Live := Live + 1;
        end Initialize;
        overriding procedure Finalize (Object : in out Guard) is
        begin
            Live := Live - 1;
        end Finalize;
    begin
        declare
            type Link is access Guard'Class;
            procedure Free is new Ada.Unchecked_Deallocation (Guard'Class, Link);
            A, B : Link := new Guard;
        begin
            Check (Live = 2);
            Free (A);
            Check (Live = 1);
        end;
        Check (Live = 0);
    end Limited_Allocation;
begin
    Escape;
    Exercise (3);
    Check (Objects = 0 and Resources = 0);
    Component_Only;
    Check (Objects = 0 and Resources = 0);
    Limited_Allocation;
    Put_Line ("controlled class-wide allocations passed");
end ControlledWideAllocations;
