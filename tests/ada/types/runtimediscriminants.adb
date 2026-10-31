with Ada.Text_IO; use Ada.Text_IO;
procedure RuntimeDiscriminants is
    Calls : Integer := 0;
    function Next return Positive is
    begin
        Calls := Calls + 1;
        return Calls;
    end Next;
    N : Positive := 3;
    type Item (D : Positive := 1) is record
        Value : Integer := D;
    end record;
    A, B : Item (Next);
    subtype Fixed is Item (N);
    F : Fixed;
    subtype Fixed_Alias is Fixed;
    type Items is array (1 .. 2) of Item (Next);
    L : Items;
    type Holder is record
        Part : Item (Next);
    end record;
    H : Holder;
    type Fixed_Ref is access Fixed;
    type Ref is access Item;
    P : Ref;
    Q : Fixed_Ref;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then raise Program_Error; end if;
    end Check;
    procedure Set (Target : in out Item; Value : Item) is
    begin
        Target := Value;
    end Set;
    procedure Needs_Fixed (Target : in out Fixed) is
    begin
        Check (Target'Constrained and Target.D = 3);
    end Needs_Fixed;
    procedure Alias_Fixed (Target : in out Fixed_Alias);
    procedure Alias_Fixed (Target : in out Fixed) is
    begin
        Check (Target.D = 3);
    end Alias_Fixed;
    type Variant (Kind : Boolean) is record
        case Kind is
            when False => Small : Integer := 10;
            when True => Large : Integer := 20;
        end case;
    end record;
    Flag : Boolean := True;
    V : Variant (Flag);
    Relation : Variant (N > 0);
    function Small_Of (X : Variant) return Integer is
    begin
        return X.Small;
    end Small_Of;
begin
    Check (Calls = 4 and A.D = 1 and B.D = 2 and L (1).D = 3 and L (2).D = 3 and H.Part.D = 4);
    Check (F.D = 3 and F.Value = 3 and F'Constrained);
    N := 7;
    declare
        G : Fixed;
        type Derived is new Fixed;
        D : Derived;
    begin
        Check (G.D = 3 and D.D = 3);
    end;
    Needs_Fixed (F);
    Alias_Fixed (F);
    begin
        Needs_Fixed (B);
        raise Program_Error;
    exception
        when Constraint_Error => Check (B.D = 2);
    end;
    begin
        Set (A, B);
        raise Program_Error;
    exception
        when Constraint_Error => Check (A.D = 1 and A.Value = 1);
    end;
    begin
        declare
            Bad : Fixed := (N, 0);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => null;
    end;
    for K in 5 .. 7 loop
        P := new Item (Next);
        Check (P.D = K and Calls = K);
    end loop;
    Q := new Item'(3, 42);
    Check (Q.D = 3 and Q.Value = 42);
    begin
        Q := new Item'(N, 99);
        raise Program_Error;
    exception
        when Constraint_Error => Check (Q.D = 3 and Q.Value = 42);
    end;
    declare
        type Pair (Left, Right : Positive) is null record;
        Grouped : Pair (Left | Right => Next);
        Named : Pair (Right => Next, Left => Next);
        Static_Named : Pair (Right => 2, Left => 1);
    begin
        Check (Calls = 11 and Grouped.Left + Grouped.Right = 17
            and Named.Left + Named.Right = 21 and Static_Named.Left = 1);
    end;
    Check (V.Kind and V.Large = 20 and Relation.Kind and Relation.Large = 20);
    begin
        Check (Small_Of (V) = 10);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    declare
        High : Integer := 5;
        subtype Discrete is Integer range 1 .. High;
        type Checked (D : Discrete) is null record;
    begin
        begin
            declare
                subtype Invalid is Checked (6);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
    end;
    Put_Line ("runtime discriminants ok");
end RuntimeDiscriminants;
