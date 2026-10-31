with Ada.Containers; use Ada.Containers;
with Ada.Containers.Vectors;
with Ada.Containers.Doubly_Linked_Lists;
with Ada.Containers.Hashed_Maps;
with Ada.Containers.Ordered_Maps;
with Ada.Containers.Hashed_Sets;
with Ada.Containers.Ordered_Sets;
with Ada.Containers.Indefinite_Vectors;
with Ada.Containers.Indefinite_Doubly_Linked_Lists;
with Ada.Containers.Indefinite_Hashed_Maps;
with Ada.Containers.Indefinite_Ordered_Maps;
with Ada.Containers.Indefinite_Hashed_Sets;
with Ada.Containers.Indefinite_Ordered_Sets;
with Ada.Text_IO;
procedure ContainerFamilies is
    function Hash (Key : Integer) return Hash_Type is begin return Hash_Type (Key mod 7); end Hash;
    function Hash (Key : String) return Hash_Type is begin return Hash_Type (Key'Length mod 7); end Hash;
    package DV is new Ada.Containers.Vectors (Positive, Integer);
    package DL is new Ada.Containers.Doubly_Linked_Lists (Integer);
    package DHM is new Ada.Containers.Hashed_Maps (Integer, Integer, Hash, "=");
    package DOM is new Ada.Containers.Ordered_Maps (Integer, Integer);
    package DHS is new Ada.Containers.Hashed_Sets (Integer, Hash, "=");
    package DOS is new Ada.Containers.Ordered_Sets (Integer);
    package IV is new Ada.Containers.Indefinite_Vectors (Positive, String);
    package IL is new Ada.Containers.Indefinite_Doubly_Linked_Lists (String);
    package IHM is new Ada.Containers.Indefinite_Hashed_Maps (String, String, Hash, "=");
    package IOM is new Ada.Containers.Indefinite_Ordered_Maps (String, String);
    package IHS is new Ada.Containers.Indefinite_Hashed_Sets (String, Hash, "=");
    package IOS is new Ada.Containers.Indefinite_Ordered_Sets (String);
    A : DV.Vector; B : DL.List; C : DHM.Map; D : DOM.Map; E : DHS.Set; F : DOS.Set;
    G : IV.Vector; H : IL.List; I : IHM.Map; J : IOM.Map; K : IHS.Set; L : IOS.Set;
begin
    DV.Append (A, 1); DL.Append (B, 1); DHM.Insert (C, 1, 1); DOM.Insert (D, 1, 1);
    DHS.Insert (E, 1); DOS.Insert (F, 1);
    IV.Append (G, "hello"); IL.Append (H, "hello"); IHM.Insert (I, "hello", "world");
    IOM.Insert (J, "hello", "world"); IHS.Insert (K, "hello"); IOS.Insert (L, "hello");
    if DV.Element (A, 1) /= 1 or DL.Element (DL.First (B)) /= 1 or
        DHM.Element (C, 1) /= 1 or DOM.Element (D, 1) /= 1 or
        not DHS.Contains (E, 1) or not DOS.Contains (F, 1) or
        IV.Element (G, 1) /= "hello" or IL.Element (IL.First (H)) /= "hello" or
        IHM.Element (I, "hello") /= "world" or IOM.Element (J, "hello") /= "world" or
        not IHS.Contains (K, "hello") or not IOS.Contains (L, "hello") then
        raise Program_Error;
    end if;
    for N in 2 .. 200 loop
        DHM.Insert (C, N, N * 3);
        DOM.Insert (D, N, N * 3);
        DHS.Insert (E, N);
        DOS.Insert (F, N);
    end loop;
    for N in 2 .. 200 loop
        if DHM.Element (C, N) /= N * 3 or DOM.Element (D, N) /= N * 3 or
            not DHS.Contains (E, N) or not DOS.Contains (F, N) then
            raise Program_Error with "associative lookup";
        end if;
    end loop;
    -- Delete in a permutation to exercise rotations and collision chains.
    for N in 1 .. 200 loop
        declare
            Key : Integer := ((N * 73) mod 200) + 1;
        begin
            DHM.Delete (C, Key);
            DOM.Delete (D, Key);
            DHS.Delete (E, Key);
            DOS.Delete (F, Key);
        end;
    end loop;
    if not DHM.Is_Empty (C) or not DOM.Is_Empty (D) or
        not DHS.Is_Empty (E) or not DOS.Is_Empty (F) then raise Program_Error; end if;
    IHM.Replace (I, "hello", "a much longer value");
    IOM.Replace (J, "hello", "x");
    if IHM.Element (I, "hello") /= "a much longer value" or IOM.Element (J, "hello") /= "x" then
        raise Program_Error with "indefinite replacement";
    end if;
    declare
        Copy : IOM.Map := J;
    begin
        IOM.Replace (Copy, "hello", "changed");
        if IOM.Element (J, "hello") /= "x" then raise Program_Error; end if;
    end;
    Ada.Text_IO.Put_Line ("container families passed");
end ContainerFamilies;
