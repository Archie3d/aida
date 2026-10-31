with Ada.Containers; use Ada.Containers;
with Ada.Containers.Indefinite_Doubly_Linked_Lists;
with Ada.Containers.Indefinite_Vectors;
with Ada.Containers.Ordered_Maps;
with Ada.Containers.Hashed_Sets;
with Ada.Text_IO;
procedure ContainerGenerics is
    generic
        type Element (<>) is private;
        with function Make (Value : Integer) return Element;
    package Library is
        package Lists is new Ada.Containers.Indefinite_Doubly_Linked_Lists (Element);
        procedure Test;
    end Library;
    package body Library is
        procedure Test is
            A, B : Lists.List;
        begin
            Lists.Append (A, Make (1));
            Lists.Append (A, Make (2));
            B := A;
            Lists.Delete_First (A);
            if Lists.Length (B) /= 2 or Lists.First_Element (B) /= Make (1) then raise Program_Error; end if;
        end Test;
    end Library;
    function Make (Value : Integer) return String is
    begin
        return Integer'Image (Value);
    end;
    package Strings is new Library (String, Make);
    type Item is record K, Payload : Integer; end record;
    function "<" (Left, Right : Item) return Boolean is begin return Left.K < Right.K; end;
    function Same (Left, Right : Item) return Boolean is begin return Left.K = Right.K; end;
    function Hash (Element : Item) return Hash_Type is begin return Hash_Type (Element.K mod 4); end;
    function Equal_Value (Left, Right : Item) return Boolean is begin return Left.Payload mod 10 = Right.Payload mod 10; end;
    package Sets is new Ada.Containers.Hashed_Sets (Item, Hash, Same, Equal_Value);
    package Maps is new Ada.Containers.Ordered_Maps (Integer, Item, "<", Equal_Value);
    use type Sets.Set;
    use type Maps.Map;
    A, B : Sets.Set;
    M, N : Maps.Map;
    subtype Word is String (1 .. 3);
    package Words is new Ada.Containers.Ordered_Maps (Word, Word);
    Word_Map : Words.Map;
    type Root is tagged record X : Integer := 0; end record;
    type Child is new Root with record Y : Integer := 0; end record;
    package Polymorphic is new Ada.Containers.Indefinite_Vectors (Positive, Root'Class);
    Values, Values_Copy : Polymorphic.Vector;
    Root_Value : Root := (X => 3);
    Child_Value : Child := (Root_Value with Y => 9);
begin
    Strings.Test;
    Words.Insert (Word_Map, "key", "val");
    Words.Replace (Word_Map, "key", "new");
    if Words.Element (Word_Map, "key") /= "new" then raise Program_Error; end if;
    Sets.Insert (A, (K => 1, Payload => 11));
    Sets.Insert (B, (K => 1, Payload => 21));
    if A /= B then raise Program_Error with "set equality actual"; end if;
    Sets.Replace (B, (K => 1, Payload => 22));
    if A = B or not Sets.Equivalent_Sets (A, B) then raise Program_Error; end if;
    Maps.Insert (M, 1, (K => 3, Payload => 11));
    Maps.Insert (N, 1, (K => 4, Payload => 21));
    if M /= N then raise Program_Error with "map equality actual"; end if;
    Polymorphic.Append (Values, Root'Class (Root_Value));
    Polymorphic.Append (Values, Root'Class (Child_Value));
    Values_Copy := Values;
    Polymorphic.Clear (Values);
    if Child (Polymorphic.Element (Values_Copy, 2)).Y /= 9 or
        Polymorphic.Element (Values_Copy, 1).X /= 3 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("container generics passed");
end ContainerGenerics;
