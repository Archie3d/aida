with Ada.Containers; use Ada.Containers;
with Ada.Containers.Vectors;
with Ada.Strings.Unbounded; use Ada.Strings.Unbounded;
with Ada.Text_IO;
procedure BasicVectors is
    package Ints is new Ada.Containers.Vectors (Positive, Integer);
    use Ints;
    subtype Small_Index is Integer range -3 .. 0;
    package Small is new Ada.Containers.Vectors (Small_Index, Integer);
    subtype High_Index is Long_Integer range Long_Integer'Last - 2 .. Long_Integer'Last;
    package High is new Ada.Containers.Vectors (High_Index, Integer);
    H : High.Vector;
    package Texts is new Ada.Containers.Vectors (Natural, Unbounded_String);
    A, B : Vector;
    S : Small.Vector;
    T : Texts.Vector;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error with "vector check";
        end if;
    end Check;
begin
    Check (Is_Empty (A) and A = Empty_Vector and Last_Index (A) = No_Index);
    Reserve_Capacity (A, 16);
    Check (Capacity (A) >= 16 and Length (A) = 0);
    for I in 1 .. 100 loop
        Append (A, I);
    end loop;
    Check (Length (A) = 100 and First_Index (A) = 1 and Last_Index (A) = 100);
    Check (First_Element (A) = 1 and Last_Element (A) = 100);
    B := A;
    Replace_Element (B, 1, 999);
    Check (Element (A, 1) = 1 and Element (B, 1) = 999 and A /= B);
    A := A;
    Append (A, A);
    Check (Length (A) = 200 and Element (A, 101) = 1);
    Clear (A);
    Check (Length (A) = 0 and Capacity (A) >= 200);
    Append (A, 2, 3);
    Prepend (A, 1);
    Insert (A, 3, 9, 2);
    Check (Length (A) = 6 and Element (A, 1) = 1 and Element (A, 4) = 9);
    Delete (A, 3, 2);
    Delete_Last (A);
    Delete_First (A);
    Check (Length (A) = 2 and First_Element (A) = 2);
    A := To_Vector (7, 3);
    Replace_Element (A, 2, 8);
    Swap (A, 1, 2);
    Reverse_Elements (A);
    Check (First_Element (A) = 7 and Last_Element (A) = 8);
    Move (A, A);
    Move (B, A);
    Check (Length (B) = 3 and Is_Empty (A));
    Set_Length (B, 1);
    Set_Length (B, 4);
    Check (Length (B) = 4 and First_Element (B) = 7);
    Delete_Last (B, 100);
    Delete_First (B);
    Check (Is_Empty (B));
    High.Append (H, 8, 3);
    Check (High.Last_Index (H) = Long_Integer'Last and High.Last_Element (H) = 8);
    High.Delete_First (H);
    Check (High.Last_Index (H) = Long_Integer'Last - 1);
    Small.Append (S, 4, 4);
    Check (Small.First_Index (S) = -3 and Small.Last_Index (S) = 0);
    Small.Insert (S, 1, 5, 0);
    Small.Delete (S, 1);
    begin
        Small.Append (S, 5);
        raise Program_Error;
    exception
        when Constraint_Error => Check (Small.Length (S) = 4);
    end;
    begin
        Small.Reserve_Capacity (S, 5);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    begin
        Replace_Element (A, 1, 8);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    begin
        Delete (B, 2);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    begin
        Check (First_Element (A) = 0);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    Texts.Append (T, To_Unbounded_String ("first"));
    Texts.Append (T, To_Unbounded_String ("second"));
    declare
        Copy : Texts.Vector := T;
    begin
        Texts.Replace_Element (Copy, 0, To_Unbounded_String ("changed"));
        Check (Texts.Element (T, 0) = "first" and Texts.Element (Copy, 0) = "changed");
    end;
    Ada.Text_IO.Put_Line ("basic vectors passed");
end BasicVectors;
