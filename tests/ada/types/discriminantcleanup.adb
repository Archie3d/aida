with Ada.Finalization;
with Ada.Text_IO; use Ada.Text_IO;
with Ada.Unchecked_Deallocation;
procedure DiscriminantCleanup is
    Live : Integer := 0;
    Fail : Boolean := False;
    type Guard is new Ada.Finalization.Controlled with null record;
    overriding procedure Initialize (X : in out Guard);
    overriding procedure Adjust (X : in out Guard);
    overriding procedure Finalize (X : in out Guard);
    overriding procedure Initialize (X : in out Guard) is
    begin
        Live := Live + 1;
    end Initialize;
    overriding procedure Adjust (X : in out Guard) is
    begin
        Live := Live + 1;
    end Adjust;
    overriding procedure Finalize (X : in out Guard) is
    begin
        Live := Live - 1;
    end Finalize;
    function Last return Integer is
    begin
        if Fail then raise Constraint_Error; end if;
        return 9;
    end Last;
    N : Positive := 3;
    type Item (D : Natural := 2) is record
        Common : Guard;
        First : String (1 .. D) := (others => 'a');
        Fixed : String (1 .. N) := (others => 'b');
        Second : String (1 .. D) := (others => 'c');
        Final : Integer := Last;
    end record;
    type Ref is access Item;
    procedure Free is new Ada.Unchecked_Deallocation (Item, Ref);
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then raise Program_Error; end if;
    end Check;
    function Make (Length : Natural) return Item is
        R : Item (Length);
    begin
        return R;
    end Make;
    type Variant (Size : Natural := 2; Kind : Boolean := False) is record
        Fixed : String (1 .. N) := (others => 'f');
        case Kind is
            when False => Text : String (1 .. Size) := (others => 'v');
            when True => Other : String (1 .. Size) := (others => 'w');
        end case;
    end record;
begin
    for Iteration in 1 .. 20 loop
        declare
            A : Item;
            B : Item (4);
            P : Ref := new Item (5);
            V : Variant;
        begin
            Check (Live = 3 and A.First = "aa" and A.Fixed = "bbb");
            A := B;
            Check (Live = 3 and A.First = "aaaa" and A.Second = "cccc");
            A.First (1) := 'z';
            Check (B.First = "aaaa");
            A := Make (6);
            Check (Live = 3 and A.First'Length = 6 and A.Fixed = "bbb");
            Free (P);
            Check (Live = 2);
            V := (Size => 3, Kind => True, Fixed => <>, Other => (others => 'q'));
            Check (V.Kind and V.Other = "qqq" and V.Fixed = "fff");
            V := (Size => 0, Kind => False, Fixed => <>, Text => (others => 'q'));
            Check (not V.Kind and V.Text'Length = 0);
            Fail := True;
            begin
                declare
                    Broken : Item (7);
                begin
                    raise Program_Error;
                end;
            exception
                when Constraint_Error => Check (Live = 2);
            end;
            begin
                P := new Item (8);
                raise Program_Error;
            exception
                when Constraint_Error => Check (Live = 2 and P = null);
            end;
            Fail := False;
        end;
        Check (Live = 0);
    end loop;
    declare
        type Wide is range -9000000000000000000 .. 9000000000000000000;
        type Values is array (Wide range <>) of Character;
        type Huge (Short : Natural; Long : Wide) is record
            First : String (1 .. Short) := (others => 'h');
            Data : Values (Wide'First .. Long);
        end record;
    begin
        begin
            declare
                H : Huge (2, Wide'Last);
            begin
                raise Program_Error;
            end;
        exception
            when Storage_Error => null;
        end;
    end;
    Put_Line ("discriminant cleanup ok");
end DiscriminantCleanup;
