with Ada.Containers; use Ada.Containers;
with Ada.Containers.Vectors;
with Ada.Containers.Doubly_Linked_Lists;
with Ada.Containers.Ordered_Maps;
with Ada.Containers.Hashed_Sets;
with Ada.Text_IO;
procedure ContainerCursors is
    package V is new Ada.Containers.Vectors (Natural, Integer);
    package L is new Ada.Containers.Doubly_Linked_Lists (Integer);
    use type L.Cursor;
    package M is new Ada.Containers.Ordered_Maps (Integer, Integer);
    function Hash (Item : Integer) return Hash_Type is begin return Hash_Type (Item mod 2); end;
    package S is new Ada.Containers.Hashed_Sets (Integer, Hash, "=");
    A, B : V.Vector;
    X, Y : L.List;
    Map : M.Map;
    Set : S.Set;
    P : V.Cursor;
    Q : L.Cursor;
    Visits, Caught : Natural := 0;
    procedure Check (Good : Boolean) is
    begin
        if not Good then raise Program_Error with "cursor check"; end if;
    end Check;
    procedure Illegal_Delete (Position : V.Cursor) is
    begin
        begin
            V.Delete_First (A);
            raise Constraint_Error;
        exception
            when Program_Error => Caught := Caught + 1;
        end;
        -- Cursor-only iteration allows replacing values.
        V.Replace_Element (A, Position, V.Element (Position) + 1);
        Visits := Visits + 1;
    end Illegal_Delete;
    procedure Illegal_Replace (Element : Integer) is
    begin
        begin
            V.Replace_Element (A, 0, 77);
            raise Constraint_Error;
        exception
            when Program_Error => Caught := Caught + 1;
        end;
    end Illegal_Replace;
    procedure Clear_Cursor (Element : Integer) is
    begin
        P := V.No_Element;
    end;
    procedure Fail_Query (Element : Integer) is
    begin
        raise Constraint_Error;
    end Fail_Query;
    procedure Bump (Element : in out Integer) is
    begin
        Element := Element + 10;
    end Bump;
    procedure Check_Map (Position : M.Cursor) is
    begin
        M.Replace (Map, M.Key (Position), 100);
        begin
            M.Include (Map, 99, 99);
            raise Constraint_Error;
        exception
            when Program_Error => Caught := Caught + 1;
        end;
    end Check_Map;
    procedure Check_Set (Position : S.Cursor) is
    begin
        begin
            S.Replace_Element (Set, Position, 90);
            raise Constraint_Error;
        exception
            when Program_Error => Caught := Caught + 1;
        end;
    end Check_Set;
begin
    V.Append (A, 1);
    V.Append (A, 2);
    P := V.First (A);
    Check (V.To_Index (P) = 0 and V.Element (V.Next (P)) = 2);
    V.Iterate (A, Illegal_Delete'Access);
    Check (Visits = 2 and Caught = 2 and V.Length (A) = 2);
    V.Query_Element (P, Illegal_Replace'Access);
    Check (Caught = 3);
    begin
        V.Query_Element (P, Fail_Query'Access);
    exception
        when Constraint_Error => null;
    end;
    V.Query_Element (P, Clear_Cursor'Access);
    Check (not V.Has_Element (P));
    P := V.First (A);
    V.Update_Element (A, P, Bump'Access);
    Check (V.Element (A, 0) = 12);
    P := V.Last (A);
    V.Insert (A, P, 9, P, 0);
    Check (V.To_Index (P) = 1);
    V.Insert (A, P, 9, P, 2);
    Check (V.Element (P) = 9 and V.Length (A) = 4);
    V.Delete (A, P, 2);
    P := V.First (A);
    B := A;
    begin
        V.Replace_Element (B, P, 8);
        raise Constraint_Error;
    exception
        when Program_Error => null;
    end;
    begin
        Check (V.Element (V.No_Element) = 0);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    V.Delete (A, P);
    Check (not V.Has_Element (P) and V.Length (A) = 1);
    L.Append (X, 3);
    L.Append (X, 1);
    L.Append (X, 2);
    Q := L.First (X);
    L.Swap_Links (X, Q, L.Last (X));
    Check (L.Element (Q) = 3 and Q = L.Last (X));
    L.Splice (Y, L.No_Element, X, Q);
    Check (L.Element (Q) = 3 and Q = L.First (Y));
    L.Splice (Y, Q, X);
    Check (L.Is_Empty (X) and L.Length (Y) = 3 and L.Element (Q) = 3);
    L.Splice (Y, L.First (Y), Q);
    Check (L.First (Y) = Q);
    M.Insert (Map, 1, 2);
    M.Iterate (Map, Check_Map'Access);
    Check (M.Element (Map, 1) = 100 and Caught = 4);
    S.Insert (Set, 1);
    S.Iterate (Set, Check_Set'Access);
    Check (S.Contains (Set, 1) and Caught = 5);
    Ada.Text_IO.Put_Line ("container cursors passed");
end ContainerCursors;
