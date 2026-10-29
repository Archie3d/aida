with Ada.Containers; use Ada.Containers;
with Ada.Containers.Ordered_Sets;
with Ada.Containers.Hashed_Sets;
with Ada.Text_IO;
procedure ContainerSets is
    type Item is record
        K : Integer;
        Value : Integer;
    end record;
    function "<" (Left, Right : Item) return Boolean is begin return Left.K < Right.K; end;
    Fail_Key : Boolean := False;
    function Key (Element : Item) return Integer is
    begin
        if Fail_Key then raise Constraint_Error; end if;
        return Element.K;
    end;
    function Hash (Element : Item) return Hash_Type is begin return Hash_Type (Element.K mod 3); end;
    function Hash (Key : Integer) return Hash_Type is begin return Hash_Type (Key mod 3); end;
    function Equivalent (Left, Right : Item) return Boolean is begin return Left.K = Right.K; end;
    package O is new Ada.Containers.Ordered_Sets (Item);
    package OK is new O.Generic_Keys (Integer, Key);
    package H is new Ada.Containers.Hashed_Sets (Item, Hash, Equivalent);
    package HK is new H.Generic_Keys (Integer, Key, Hash, "=");
    use type O.Set;
    use type H.Set;
    A, B, C : O.Set;
    X, Y, Z : H.Set;
    P : O.Cursor;
    HP : H.Cursor;
    Inserted : Boolean;
    procedure Check (Good : Boolean) is
    begin
        if not Good then raise Program_Error with "set check"; end if;
    end;
    procedure Bump (Element : in out Item) is begin Element.Value := Element.Value + 1; end;
    procedure Change_Key (Element : in out Item) is begin Element.K := 99; end;
begin
    for I in 1 .. 5 loop
        O.Insert (A, (I, I * 10));
        H.Insert (X, (I, I * 10));
    end loop;
    for I in 4 .. 8 loop
        O.Insert (B, (I, I * 10));
        H.Insert (Y, (I, I * 10));
    end loop;
    C := A or B;
    Z := X or Y;
    Check (O.Length (C) = 8 and H.Length (Z) = 8);
    Check (O.Length (A and B) = 2 and H.Length (X and Y) = 2);
    Check (O.Length (A - B) = 3 and H.Length (X - Y) = 3);
    Check (O.Length (A xor B) = 6 and H.Length (X xor Y) = 6);
    Check (O.Is_Subset (A, C) and H.Is_Subset (X, Z) and O.Overlap (A, B) and H.Overlap (X, Y));
    O.Symmetric_Difference (C, C);
    H.Difference (Z, Z);
    Check (O.Is_Empty (C) and H.Is_Empty (Z));
    O.Insert (A, (1, 100), P, Inserted);
    Check (not Inserted and OK.Element (A, 1).Value = 10);
    O.Include (A, (1, 100));
    H.Include (X, (1, 100));
    Check (OK.Element (A, 1).Value = 100 and HK.Element (X, 1).Value = 100);
    O.Replace_Element (A, P, (9, 900));
    Check (not OK.Contains (A, 1) and OK.Contains (A, 9) and O.Last_Element (A).K = 9);
    begin
        O.Replace_Element (A, P, (2, 20));
        raise Constraint_Error;
    exception
        when Program_Error => Check (OK.Contains (A, 9));
    end;
    P := OK.Find (A, 9);
    OK.Update_Element_Preserving_Key (A, P, Bump'Access);
    Check (OK.Element (A, 9).Value = 901);
    begin
        OK.Update_Element_Preserving_Key (A, P, Change_Key'Access);
        raise Constraint_Error;
    exception
        when Program_Error => Check (not OK.Contains (A, 9) and not OK.Contains (A, 99));
    end;
    HP := HK.Find (X, 1);
    HK.Update_Element_Preserving_Key (X, HP, Bump'Access);
    Check (HK.Element (X, 1).Value = 101);
    begin
        HK.Update_Element_Preserving_Key (X, HP, Change_Key'Access);
        raise Constraint_Error;
    exception
        when Program_Error => Check (not HK.Contains (X, 1) and not HK.Contains (X, 99));
    end;
    Check (OK.Key (OK.Floor (A, 3)) = 3 and OK.Key (OK.Ceiling (A, 3)) = 3);
    Check (O.Equivalent_Sets (A, A) and H.Equivalent_Sets (X, X));
    C := A;
    O.Replace (C, (2, 999));
    Check (O.Equivalent_Sets (A, C) and A /= C);
    Z := X;
    H.Replace (Z, (2, 999));
    Check (H.Equivalent_Sets (X, Z) and X /= Z);
    P := O.First (C);
    Fail_Key := True;
    begin
        OK.Update_Element_Preserving_Key (A, P, Bump'Access);
        raise Constraint_Error;
    exception
        when Program_Error => null;
    end;
    Fail_Key := False;
    Ada.Text_IO.Put_Line ("container sets passed");
end ContainerSets;
