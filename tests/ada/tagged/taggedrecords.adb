with Ada.Text_IO; use Ada.Text_IO;
procedure TaggedRecords is
    package Shapes is
        type Root is tagged record
            X : Integer := 2;
        end record;
        procedure Bump (Item : in out Root);
        procedure Reset (Item : out Root);
        function Read (Item : Root) return Integer;
        type Child is new Root with record
            Y : Long_Integer := 30;
        end record;
        overriding function Read (Item : Child) return Integer;
        type Leaf is new Child with record
            Z : Integer := 40;
        end record;
        type Empty is tagged null record;
        type Empty_Child is new Empty with null record;
    end Shapes;
    package body Shapes is
        procedure Bump (Item : in out Root) is
        begin
            Item.X := Item.X + 1;
        end Bump;
        procedure Reset (Item : out Root) is
        begin
            Item := (X => 9);
        end Reset;
        function Read (Item : Root) return Integer is
        begin
            return Item.X;
        end Read;
        function Read (Item : Child) return Integer is
        begin
            return Item.X + Integer (Item.Y);
        end Read;
    end Shapes;
    use Shapes;
    A : Child;
    B : Child := (Root'(X => 5) with Y => 60);
    C : Leaf := (Root with Y => 70, Z => 80);
    E : Empty := (null record);
    F : Empty_Child := (E with null record);
    type Link is access Leaf;
    Pointer : Link := new Leaf'(Child'(Root'(X => 1) with Y => 2) with Z => 3);
    Parent_Copy : Root := Root (C);
    Parent_View : Root renames Root (C);
    function Measure (Item : Root) return Integer is
    begin
        return 1;
    end Measure;
    function Measure (Item : Child) return Integer is
    begin
        return Integer (Item.Y);
    end Measure;
    function Make return Child is
    begin
        return (Root with Y => 100);
    end Make;
begin
    Put_Line (Integer'Image (Read (A)));
    Bump (A);
    Put_Line (Integer'Image (Read (A)));
    Put_Line (Integer'Image (Read (B)));
    Put_Line (Integer'Image (Read (C)));
    Bump (Root (C));
    Parent_View.X := 4;
    Put_Line (Integer'Image (Parent_Copy.X));
    Put_Line (Integer'Image (C.X));
    Root (C) := Root'(X => 11);
    Put_Line (Integer'Image (C.X + Integer (C.Y) + C.Z));
    Reset (Root (C));
    Put_Line (Integer'Image (C.X + Integer (C.Y) + C.Z));
    Pointer.X := 10;
    Bump (Root (Pointer.all));
    Put_Line (Integer'Image (Pointer.X + Integer (Pointer.Y) + Pointer.Z));
    A := Make;
    Put_Line (Integer'Image (Read (A)));
    Put_Line (Boolean'Image (E = Empty (F)));
    Put_Line (Integer'Image (Measure ((Root with Y => 123))));
    Put_Line (Integer'Image (Root'Size));
    Put_Line (Integer'Image (Child'Size));
    Put_Line (Integer'Image (Leaf'Size));
end TaggedRecords;
