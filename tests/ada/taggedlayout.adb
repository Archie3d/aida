with Ada.Text_IO; use Ada.Text_IO;
with Tagged_Layout; use Tagged_Layout;
procedure TaggedLayout is
    function Tag_Of (Object : System.Address) return System.Address;
    pragma Import (C, Tag_Of, "tagOf");
    function Valid_Descriptor (Object : System.Address; Size : Long_Integer;
                               Parent : System.Address) return Integer;
    pragma Import (C, Valid_Descriptor, "validDescriptor");
    type Child is new Root with record
        Y : Integer := 10;
    end record;
    overriding function Make return Child is
    begin
        return (Root with Y => 11);
    end Make;
    type Leaf is new Child with record
        Z : Long_Integer := 12;
    end record;
    overriding function Make return Leaf is
    begin
        return (Child'(Root with Y => 13) with Z => 14);
    end Make;
    type Null_Child is new Root with null record;
    type Root_Link is access Root;
    type Leaf_Link is access Leaf;
    A : Root;
    B : Child;
    C : Leaf;
    D : Null_Child := Make;
    Original_Tag : System.Address := Tag_Of (C'Address);
    P : Leaf_Link := new Leaf;
    Q : Leaf_Link := new Leaf'(Child'(Root with Y => 4) with Z => 5);
    R : Root := Root (C);
    type Items is array (Positive range <>) of Leaf;
    List : Items (1 .. 2) := (others => Make);
    Default_List : Items (1 .. 2);
    type Holder is record
        Item : Leaf;
    end record;
    H : Holder;
    H2 : Holder := (Item => Make);
    generic
        Offset : Integer;
    package G is
        type T is new Root with null record;
        Object : T := Make;
    end G;
    package First is new G (1);
    package Second is new G (2);
    package Hidden is
        type T is tagged private;
        subtype Alias is T;
        function Make return Alias;
    private
        type T is tagged record
            X : Long_Integer;
        end record;
    end Hidden;
    package body Hidden is
        function Make return Alias is
        begin
            return (X => 15);
        end Make;
    end Hidden;
    Hidden_Object : Hidden.Alias := Hidden.Make;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    function Slice (Object : Leaf) return Root is
    begin
        return Root (Object);
    end Slice;
begin
    Check (Valid_Descriptor (A'Address, 16, null) = 1);
    Check (Valid_Descriptor (Hidden_Object'Address, 16, null) = 1);
    Check (Valid_Descriptor (B'Address, 24, Tag_Of (A'Address)) = 1);
    Check (Valid_Descriptor (C'Address, 32, Tag_Of (B'Address)) = 1);
    Check (Tag_Of (D'Address) /= Tag_Of (A'Address));
    Check (Valid_Descriptor (D'Address, 16, Tag_Of (A'Address)) = 1);
    Check (Tag_Of (Root (C)'Address) = Original_Tag);
    Check (Tag_Of (R'Address) = Tag_Of (A'Address));
    Reset (Root (C));
    Check (Tag_Of (C'Address) = Original_Tag and C.Y = 10 and C.Z = 12);
    Root (C) := (X => 22);
    Check (Tag_Of (C'Address) = Original_Tag and C.Y = 10 and C.Z = 12);
    C := Make;
    Check (Tag_Of (C'Address) = Original_Tag and C.Y = 13 and C.Z = 14);
    R := Slice (C);
    Check (Tag_Of (R'Address) = Tag_Of (A'Address));
    Check (Tag_Of (P.all'Address) = Original_Tag);
    Check (Tag_Of (Q.all'Address) = Original_Tag);
    Check (Tag_Of (List (2)'Address) = Original_Tag);
    Check (Tag_Of (Default_List (2)'Address) = Original_Tag);
    Check (Tag_Of (H.Item'Address) = Original_Tag);
    Check (Tag_Of (H2.Item'Address) = Original_Tag);
    Check (Tag_Of (First.Object'Address) /= Tag_Of (Second.Object'Address));
    Check (Valid_Descriptor (First.Object'Address, 16, Tag_Of (A'Address)) = 1);
    declare
        Count : Integer := 3;
        Dynamic_List : Items (1 .. Count) := (others => Make);
    begin
        Check (Tag_Of (Dynamic_List (3)'Address) = Original_Tag);
    end;
    Put_Line ("tagged layout ok");
end TaggedLayout;
