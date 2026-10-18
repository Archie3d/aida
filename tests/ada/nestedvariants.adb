with Ada.Text_IO; use Ada.Text_IO;
procedure NestedVariants is
    Calls : Integer := 0;
    Checks : Integer := 0;
    function Default_Value (Value : Integer) return Integer is
    begin
        Calls := Calls + 1;
        return Value;
    end Default_Value;
    type Wide_Integer is range -900000000000 .. 900000000000;
    type Item (Outer, Inner, Deep : Boolean) is record
        Common : Integer := Default_Value (1);
        case Outer is
            when True =>
                Middle : Integer := Default_Value (2);
                case Inner is
                    when True =>
                        case Deep is
                            when True => Leaf : Integer := Default_Value (3);
                            when others => Wide : Wide_Integer := 12345678901;
                        end case;
                    when others => Other_Inner : Integer := Default_Value (4);
                end case;
            when others => Other_Outer : Integer := Default_Value (5);
        end case;
    end record;
    A : Item (True, True, True);
    B : Item (True, True, False);
    C : Item (True, False, True);
    D : Item (False, True, True);
    function Leaf_Of (X : Item) return Integer is
    begin
        return X.Leaf;
    end Leaf_Of;
    function Copy (X : Item) return Item is
    begin
        return X;
    end Copy;
    procedure Replace (Target : in out Item; Source : Item) is
    begin
        Target := Source;
    end Replace;
    procedure Check (Condition : Boolean) is
    begin
        Checks := Checks + 1;
        if not Condition then
            Put_Line ("failed check" & Integer'Image (Checks));
            raise Program_Error with "nested variant check 1";
        end if;
    end Check;
begin
    Check (Calls = 10);
    Check (Item'Size = 192);
    Check (A.Common = 1 and A.Middle = 2 and A.Leaf = 3);
    Check (B.Wide = 12345678901 and C.Other_Inner = 4 and D.Other_Outer = 5);
    Check (Leaf_Of (A) = 3);
    begin
        Check (Leaf_Of (B) = 0);
        raise Program_Error with "nested variant check 2";
    exception
        when Constraint_Error => null;
    end;
    begin
        Check (Leaf_Of (C) = 0);
        raise Program_Error with "nested variant check 3";
    exception
        when Constraint_Error => null;
    end;
    begin
        Check (Leaf_Of (D) = 0);
        raise Program_Error with "nested variant check 4";
    exception
        when Constraint_Error => null;
    end;
    declare
        E : Item (True, True, True) := Copy (A);
        F : Item (True, True, True) := (Outer => True, Inner => True, Deep => True,
                     Common => 1, Middle => 2, Leaf => 9);
        G : Item (True, True, False) := (True, True, False, 1, 2, 12345678901);
        H : Item (False, False, False) := (False, False, False, 1, 5);
        subtype Leaf_Item is Item (True, True, True);
        type Vector is array (1 .. 2) of Leaf_Item;
        Items : Vector := (A, E);
        type Wrapper is record
            Before : Integer;
            Value : Leaf_Item;
            After : Integer;
        end record;
        Wrapped : Wrapper := (11, A, 22);
    begin
        Check (E = A and E /= F and B = G and D /= H);
        Check (Items (1) = Items (2));
        Check (Wrapped.Before = 11 and Wrapped.Value.Leaf = 3 and Wrapped.After = 22);
        E := F;
        Check (E.Leaf = 9 and E = F);
        Replace (E, A);
        Check (E = A);
        Replace (E, F);
        Items (2) := E;
        Check (Items (1) /= Items (2));
        begin
            Replace (E, B);
            raise Program_Error with "nested variant check 5";
        exception
            when Constraint_Error => Check (E = F);
        end;
        begin
            Replace (E, C);
            raise Program_Error with "nested variant check 6";
        exception
            when Constraint_Error => Check (E = F);
        end;
        begin
            Replace (E, D);
            raise Program_Error with "nested variant check 7";
        exception
            when Constraint_Error => Check (E = F);
        end;
        Check (Copy (Item'(True, True, False, 1, 2, 12345678901)) = B);
        Check (Copy (B) = B and Copy (C) = C and Copy (D) = D);
    end;
    declare
        function Fail return Integer is
        begin
            raise Program_Error with "inactive nested default evaluated";
            return 0;
        end Fail;
        type Two_Sided (Outer, Inner : Boolean) is record
            case Outer is
                when True =>
                    case Inner is
                        when True => Left_Value : Integer := 11;
                        when others => null;
                    end case;
                when others =>
                    case Inner is
                        when True => Right_Value : Integer := 22;
                        when others => Never : Integer := Fail;
                    end case;
            end case;
        end record;
        Left : Two_Sided (True, True);
        Empty : Two_Sided (True, False);
        Right : Two_Sided (False, True);
        function Read_Right (X : Two_Sided) return Integer is
        begin
            return X.Right_Value;
        end Read_Right;
    begin
        Check (Left.Left_Value = 11 and Read_Right (Right) = 22);
        Check (Empty = Two_Sided'(True, False));
        Check (Right = Two_Sided'(False, True, 22));
        begin
            Check (Read_Right (Left) = 0);
            raise Program_Error with "missing outer variant check";
        exception
            when Constraint_Error => null;
        end;
    end;
    Put_Line ("nested variants passed");
end NestedVariants;
