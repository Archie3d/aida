with Ada.Finalization;
procedure NestedVariantsErrors is
    type Item (Outer, Inner : Boolean) is record
        case Outer is
            when True =>
                case Inner is
                    when True => Leaf : Integer;
                    when False => Other_Inner : Integer;
                end case;
            when False => Other_Outer : Integer;
        end case;
    end record;
    A : Item (True, False);
    B : Item (False, True);
    Missing : Item (True, True) := (True, True);
    Wrong : Item (True, True) := (True, True, Other_Inner => 1);
    Mismatch : Item (True, True) := (True, False, 1);
    type Duplicate (Outer, Inner : Boolean) is record
        case Outer is
            when True =>
                case Inner is
                    when True => null;
                    when True => null;
                    when False => null;
                end case;
            when False => null;
        end case;
    end record;
    type Missing_Choice (Outer, Inner : Boolean) is record
        case Outer is
            when True =>
                case Inner is
                    when True => null;
                end case;
            when False => null;
        end case;
    end record;
    type Bad_Selector (Outer : Boolean) is record
        Value : Boolean;
        case Outer is
            when True =>
                case Value is
                    when others => null;
                end case;
            when False => null;
        end case;
    end record;
    type Guard_Type is new Ada.Finalization.Controlled with null record;
    type Controlled_Part (Outer, Inner : Boolean) is record
        case Outer is
            when True =>
                case Inner is
                    when True => Guard : Guard_Type;
                    when others => null;
                end case;
            when others => null;
        end case;
    end record;
begin
    A.Leaf := 1;
    B.Leaf := 2;
end NestedVariantsErrors;
