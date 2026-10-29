with Ada.Containers; use Ada.Containers;
with Ada.Containers.Doubly_Linked_Lists;
with Ada.Containers.Hashed_Maps;
with Ada.Containers.Ordered_Maps;
with Controlled_Result_Model; use Controlled_Result_Model;
with Ada.Text_IO;
procedure ContainerLifetime is
    function Hash (Key : Integer) return Hash_Type is begin return Hash_Type (Key mod 2); end;
    package L is new Ada.Containers.Doubly_Linked_Lists (Guard);
    package H is new Ada.Containers.Hashed_Maps (Integer, Guard, Hash, "=");
    package O is new Ada.Containers.Ordered_Maps (Integer, Guard);
    procedure Exercise is
        List, Copy : L.List;
        Map : H.Map;
        Ordered : O.Map;
        Item : Guard;
        Before : Integer;
    begin
        L.Append (List, Item, 3);
        H.Insert (Map, 1, Item);
        O.Insert (Ordered, 1, Item);
        Check (Objects = 6 and Resources = 1);
        Copy := List;
        Check (Objects = 9);
        L.Clear (Copy);
        Before := Objects;
        Fail_Adjust := 2;
        begin
            L.Append (List, Item, 4);
            raise Constraint_Error;
        exception
            when Program_Error => null;
        end;
        Check (Objects = Before and L.Length (List) = 3);
        Fail_Adjust := 2;
        begin
            Copy := List;
            raise Constraint_Error;
        exception
            when Program_Error => null;
        end;
        Check (Objects = Before and L.Length (List) = 3);
        Fail_Adjust := 1;
        begin
            H.Insert (Map, 2, Item);
            raise Constraint_Error;
        exception
            when Program_Error => null;
        end;
        Check (Objects = Before and H.Length (Map) = 1 and not H.Contains (Map, 2));
        Fail_Adjust := 1;
        begin
            O.Insert (Ordered, 2, Item);
            raise Constraint_Error;
        exception
            when Program_Error => null;
        end;
        Check (Objects = Before and O.Length (Ordered) = 1 and not O.Contains (Ordered, 2));
        Fail_Finalize := True;
        begin
            L.Clear (List);
            raise Constraint_Error;
        exception
            when Program_Error => null;
        end;
        Check (Objects = 3 and Resources = 1 and L.Is_Empty (List));
        begin
            declare
                Temporary : O.Map := Ordered;
            begin
                Check (Objects = 4);
                raise Constraint_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        Check (Objects = 3);
    end Exercise;
begin
    Exercise;
    Check (Objects = 0 and Resources = 0);
    Ada.Text_IO.Put_Line ("container lifetime passed");
end ContainerLifetime;
