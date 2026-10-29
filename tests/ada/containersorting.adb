with Ada.Containers; use Ada.Containers;
with Ada.Containers.Vectors;
with Ada.Containers.Doubly_Linked_Lists;
with Ada.Containers.Indefinite_Vectors;
with Ada.Containers.Generic_Array_Sort;
with Ada.Containers.Generic_Constrained_Array_Sort;
with Ada.Text_IO;
procedure ContainerSorting is
    package V is new Ada.Containers.Vectors (Positive, Integer);
    package VS is new V.Generic_Sorting;
    package L is new Ada.Containers.Doubly_Linked_Lists (Integer);
    package LS is new L.Generic_Sorting;
    package IV is new Ada.Containers.Indefinite_Vectors (Positive, String);
    package ISort is new IV.Generic_Sorting;
    use type L.Cursor;
    type Index is (A, B, C, D, E);
    type Items is array (Index range <>) of Integer;
    type Fixed_Items is array (Index) of Integer;
    procedure Sort is new Ada.Containers.Generic_Array_Sort (Index, Integer, Items);
    procedure Fixed_Sort is new Ada.Containers.Generic_Constrained_Array_Sort (Index, Integer, Fixed_Items);
    subtype High_Index is Long_Integer range Long_Integer'Last - 2 .. Long_Integer'Last;
    type High_Array is array (High_Index range <>) of Integer;
    procedure High_Sort is new Ada.Containers.Generic_Array_Sort (High_Index, Integer, High_Array);
    High : High_Array (High_Index'First .. High_Index'Last) := (3, 2, 1);
    X, Y : V.Vector;
    List, Other : L.List;
    Saved : L.Cursor;
    Texts : IV.Vector;
    Array_Value : Items (B .. D) := (7, 2, 4);
    Fixed : Fixed_Items := (5, 4, 3, 2, 1);
    procedure Check (Good : Boolean) is
    begin
        if not Good then raise Program_Error with "sorting check"; end if;
    end;
begin
    for I in reverse 1 .. 103 loop
        V.Append (X, I);
        L.Append (List, I);
    end loop;
    Saved := L.First (List);
    VS.Sort (X);
    LS.Sort (List);
    Check (VS.Is_Sorted (X) and LS.Is_Sorted (List));
    Check (L.Element (Saved) = 103 and Saved = L.Last (List));
    for I in 1 .. 103 loop
        Check (V.Element (X, I) = I);
    end loop;
    for I in 1 .. 40 loop
        V.Append (Y, I * 2);
        L.Append (Other, I * 2);
    end loop;
    VS.Merge (X, Y);
    LS.Merge (List, Other);
    Check (V.Is_Empty (Y) and L.Is_Empty (Other) and V.Length (X) = 143 and L.Length (List) = 143);
    Check (VS.Is_Sorted (X) and LS.Is_Sorted (List) and L.Element (Saved) = 103);
    VS.Merge (X, X);
    LS.Merge (List, List);
    IV.Append (Texts, "zebra");
    IV.Append (Texts, "a");
    IV.Append (Texts, "middle");
    ISort.Sort (Texts);
    Check (ISort.Is_Sorted (Texts) and IV.Element (Texts, 1) = "a");
    Sort (Array_Value);
    Fixed_Sort (Fixed);
    Check (Array_Value (B) = 2 and Array_Value (D) = 7 and Fixed (A) = 1 and Fixed (E) = 5);
    High_Sort (High);
    Check (High (High_Index'First) = 1 and High (High_Index'Last) = 3);
    declare
        Empty : V.Vector;
    begin
        V.Insert_Space (Empty, 1, 5);
        V.Replace_Element (Empty, 2, 1);
        V.Replace_Element (Empty, 3, 9);
        V.Replace_Element (Empty, 4, 8);
        V.Replace_Element (Empty, 5, 7);
        begin
            VS.Sort (Empty);
            raise Program_Error;
        exception
            when Constraint_Error => Check (V.Element (Empty, 2) = 1 and V.Element (Empty, 4) = 8);
        end;
        begin
            V.Swap (Empty, 1, 2);
            raise Program_Error;
        exception
            when Constraint_Error => Check (V.Element (Empty, 2) = 1);
        end;
    end;
    Ada.Text_IO.Put_Line ("container sorting passed");
end ContainerSorting;
