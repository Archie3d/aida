with Ada.Finalization;
with Ada.Unchecked_Deallocation;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledCopies is
    type Resource is record
        References : Integer;
        Value : Integer;
    end record;
    type Link is access Resource;
    procedure Free is new Ada.Unchecked_Deallocation (Resource, Link);
    Resources : Integer := 0;
    Initializes : Integer := 0;
    Adjusts : Integer := 0;
    Finalizes : Integer := 0;
    type Guard is new Ada.Finalization.Controlled with record
        Data : Link := null;
    end record;
    overriding procedure Initialize (Object : in out Guard);
    overriding procedure Adjust (Object : in out Guard);
    overriding procedure Finalize (Object : in out Guard);
    procedure Initialize (Object : in out Guard) is
    begin
        Initializes := Initializes + 1;
        Object.Data := new Resource'(1, Initializes);
        Resources := Resources + 1;
    end Initialize;
    procedure Adjust (Object : in out Guard) is
    begin
        Adjusts := Adjusts + 1;
        Object.Data.References := Object.Data.References + 1;
    end Adjust;
    procedure Finalize (Object : in out Guard) is
    begin
        Finalizes := Finalizes + 1;
        if Object.Data = null or Object.Data.References < 1 then
            raise Program_Error;
        end if;
        Object.Data.References := Object.Data.References - 1;
        if Object.Data.References = 0 then
            Free (Object.Data);
            Resources := Resources - 1;
        end if;
        Object.Data := null;
    end Finalize;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    procedure Assign (Target : in out Guard; Source : Guard) is
    begin
        Target := Source;
    end Assign;
    type Pair is record
        First, Second : Guard;
    end record;
    type Guards is array (Positive range <>) of Guard;
    function Size return Integer is
    begin
        return 3;
    end Size;
begin
    declare
        A : Guard;
        B : Guard := A;
        C : Guard;
        Alias : Guard renames C;
        Before : Integer;
    begin
        Check (Resources = 2 and Initializes = 2 and Adjusts = 1);
        C := A;
        Check (Resources = 1 and A.Data.References = 3);
        Before := Adjusts;
        Alias := C;
        Check (Adjusts = Before);
        Assign (C, B);
        Check (Resources = 1 and A.Data.References = 3);
    end;
    Check (Resources = 0);
    Put_Line ("copies and assignment");
    declare
        A : Pair;
        B : Pair := A;
        C : Pair := (A.First, A.Second);
    begin
        Check (Resources = 2 and A.First.Data.References = 3);
        C := B;
        Check (A.Second.Data.References = 3);
        C.First := B.Second;
        Check (A.First.Data.References = 2 and A.Second.Data.References = 4);
    end;
    Check (Resources = 0);
    Put_Line ("record components");
    declare
        A : Guards (1 .. 3);
        First : Integer := A(1).Data.Value;
        Second : Integer := A(2).Data.Value;
    begin
        A(2 .. 3) := A(1 .. 2);
        Check (A(1).Data.Value = First and A(2).Data.Value = First);
        Check (A(3).Data.Value = Second and Resources = 2);
        A(1 .. 2) := A(2 .. 3);
        Check (A(1).Data.Value = First and A(2).Data.Value = Second);
    end;
    Check (Resources = 0);
    Put_Line ("overlapping slices");
    declare
        N : Integer := Size;
        A : Guard;
        B : Guards (1 .. N) := (others => A);
        C : Guards (5 .. 4 + N) := B;
    begin
        Check (Resources = 1 and A.Data.References = 7);
        C := B;
        Check (A.Data.References = 7);
        begin
            C := B(1 .. 2);
            raise Program_Error;
        exception
            when Constraint_Error => Check (A.Data.References = 7);
        end;
    end;
    Check (Resources = 0);
    Put_Line ("dynamic arrays");
    declare
        A : Guard := Guard'(Ada.Finalization.Controlled with Data => new Resource'(1, 42));
    begin
        Resources := Resources + 1;
        Check (A.Data.References = 1 and A.Data.Value = 42);
    end;
    Check (Resources = 0);
    Put_Line ("aggregate construction");
    Check (Finalizes = Initializes + Adjusts + 1);
    Put_Line ("controlled copies passed");
end ControlledCopies;
