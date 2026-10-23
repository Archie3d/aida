with Ada.Finalization;
with Ada.Text_IO; use Ada.Text_IO;
procedure DefaultDiscriminants is
    Live : Integer := 0;
    Adjustments : Integer := 0;
    Finalizations : Integer := 0;
    type Guard is new Ada.Finalization.Controlled with null record;
    overriding procedure Initialize (G : in out Guard);
    overriding procedure Adjust (G : in out Guard);
    overriding procedure Finalize (G : in out Guard);
    overriding procedure Initialize (G : in out Guard) is
    begin
        Live := Live + 1;
    end Initialize;
    overriding procedure Adjust (G : in out Guard) is
    begin
        Live := Live + 1;
        Adjustments := Adjustments + 1;
    end Adjust;
    overriding procedure Finalize (G : in out Guard) is
    begin
        Live := Live - 1;
        Finalizations := Finalizations + 1;
    end Finalize;
    type Controlled_Item (D : Boolean := False) is record
        G : Guard;
        Value : Integer := 1;
    end record;
    Calls : Integer := 0;
    function Next return Integer is
    begin
        Calls := Calls + 1;
        return Calls;
    end Next;
    type Pair (Left, Right : Integer := Next) is null record;
    A, B : Pair;
    type Wide is range -900000000000 .. 900000000000;
    type Item (Kind : Boolean := False) is record
        Common : Integer := 10;
        case Kind is
            when False => Small : Integer := 20;
            when True => Large : Wide := 12345678901;
        end case;
    end record;
    Boxed : Item := (others => <>);
    X : Item;
    Y : Item := (True, 30, 9876543210);
    Fixed : Item (False);
    C : constant Item := (True, 40, 12345678901);
    Alias : Item renames X;
    type Holder is record
        Part : Item;
    end record;
    H : Holder;
    type Holder_Ref is access Holder;
    Heap_Holder : Holder_Ref := new Holder;
    type List is array (1 .. 2) of Item;
    L : List;
    type Ref is access Item;
    P : Ref := new Item;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error with "default discriminant check";
        end if;
    end Check;
    procedure Replace (Target : in out Item; Source : Item; Constrained : Boolean) is
        Renamed : Item renames Target;
        procedure Nested is
        begin
            Check (Target'Constrained = Constrained);
            Check (Renamed'Constrained = Constrained);
            Renamed := Source;
        end Nested;
    begin
        Check (Source'Constrained);
        Nested;
    end Replace;
    procedure Fill (Target : out Item) is
    begin
        Target := (True, 50, 12345678901);
    end Fill;
    function Make return Item is
    begin
        return (True, 60, 12345678901);
    end Make;
begin
    Check (Calls = 4 and A.Left = 1 and A.Right = 2 and B.Left = 3 and B.Right = 4);
    Check (not X'Constrained and not Y'Constrained and Fixed'Constrained and C'Constrained);
    Check (X.Small = 20 and X.Common = 10 and not Boxed.Kind and Boxed.Small = 20);
    Replace (Alias, Y, False);
    Check (X.Kind and X.Large = 9876543210);
    Alias := (Kind => <>, Common => 70, Small => 80);
    Check (not X.Kind and X.Small = 80);
    begin
        Replace (Fixed, Y, True);
        raise Program_Error;
    exception
        when Constraint_Error => Check (not Fixed.Kind and Fixed.Small = 20);
    end;
    Fill (X);
    Check (X.Kind and X.Common = 50);
    begin
        Fill (Fixed);
        raise Program_Error;
    exception
        when Constraint_Error => Check (not Fixed.Kind and Fixed.Small = 20);
    end;
    H.Part := Y;
    Replace (Heap_Holder.Part, Y, False);
    Check (Heap_Holder.Part.Kind and not Heap_Holder.Part'Constrained);
    L (1) := Make;
    Check (H.Part.Kind and L (1).Kind and not H.Part'Constrained and not L (1)'Constrained);
    Check (P.all'Constrained);
    begin
        P.all := Y;
        raise Program_Error;
    exception
        when Constraint_Error => Check (not P.Kind);
    end;
    declare
        type Plain (D : Integer) is record Value : Integer; end record;
        V : Plain := (1, 10);
        W : Plain := (2, 20);
    begin
        Check (V'Constrained);
        begin
            V := W;
            raise Program_Error;
        exception
            when Constraint_Error => Check (V.D = 1 and V.Value = 10);
        end;
    end;
    declare
        Mutable : Controlled_Item;
        Source : Controlled_Item (True);
        Target : Controlled_Item (False);
        Saved_Adjustments, Saved_Finalizations : Integer;
    begin
        Mutable := Source;
        Check (Mutable.D and Live = 3);
        Saved_Adjustments := Adjustments;
        Saved_Finalizations := Finalizations;
        begin
            Target := Source;
            raise Program_Error;
        exception
            when Constraint_Error =>
                Check (not Target.D and Live = 3 and Adjustments = Saved_Adjustments
                    and Finalizations = Saved_Finalizations);
        end;
    end;
    Check (Live = 0);
    Put_Line ("default discriminants ok");
end DefaultDiscriminants;
