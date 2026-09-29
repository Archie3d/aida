with Ada.Text_IO; use Ada.Text_IO;
procedure ClasswideDispatch is
    Offset : Integer := 100;
    package Shapes is
        type Root is tagged record
            X : Integer := 2;
        end record;
        function Read (Item : Root) return Integer;
        procedure Bump (Item : in out Root);
        procedure Pair (Left, Right : in out Root);
        type Child is new Root with record
            Y : Integer := 30;
        end record;
        overriding function Read (Item : Child) return Integer;
        type Leaf is new Child with record
            Z : Integer := 40;
        end record;
    end Shapes;
    package body Shapes is
        function Read (Item : Root) return Integer is
        begin
            return Item.X;
        end Read;
        function Read (Item : Child) return Integer is
        begin
            return Item.X + Item.Y + Offset;
        end Read;
        procedure Bump (Item : in out Root) is
        begin
            Item.X := Item.X + 1;
        end Bump;
        procedure Pair (Left, Right : in out Root) is
        begin
            Left.X := Right.X + 10;
        end Pair;
    end Shapes;
    package Hidden is
        type T is tagged private;
        function Extra (Item : T) return Integer;
        overriding function Read (Item : T) return Integer;
        procedure Inspect (Item : T'Class);
    private
        type T is new Shapes.Root with record
            Y : Integer := 77;
        end record;
    end Hidden;
    package body Hidden is
        function Extra (Item : T) return Integer is
        begin
            return Item.Y;
        end Extra;
        function Read (Item : T) return Integer is
        begin
            return Item.X + Item.Y;
        end Read;
        procedure Inspect (Item : T'Class) is
        begin
            Put_Line (Integer'Image (Extra (Item)));
            Put_Line (Integer'Image (Read (Item)));
        end Inspect;
    end Hidden;
    H : Hidden.T;
    use Shapes;
    A : Root;
    B : Child;
    C : Leaf;
    D : Leaf := (Root with Y => 70, Z => 80);
    procedure Visit (Item : in out Root'Class) is
    begin
        Put_Line (Integer'Image (Item'Size));
        Bump (Item);
        Put_Line (Integer'Image (Read (Item)));
        Put_Line (Integer'Image (Read (Root (Item))));
    end Visit;
    procedure Both (Left, Right : in out Root'Class) is
    begin
        Pair (Right => Right, Left => Left);
        Put_Line ("same tag");
    exception
        when Constraint_Error => Put_Line ("different tags");
    end Both;
    procedure Views (Item : in out Root'Class) is
    begin
        Put_Line (Boolean'Image (Item in Child));
        Put_Line (Boolean'Image (Item in Child'Class));
        Put_Line (Boolean'Image (Item not in Leaf'Class));
        Child (Item).Y := 90;
        Put_Line (Integer'Image (Read (Item)));
    exception
        when Constraint_Error => Put_Line ("bad conversion");
    end Views;
    procedure Copy (Target : in out Root'Class; Source : Root'Class) is
    begin
        Target := Source;
        Put_Line ("copied");
    exception
        when Constraint_Error => Put_Line ("bad assignment");
    end Copy;
begin
    Hidden.Inspect (H);
    Visit (A);
    Visit (B);
    Visit (C);
    Both (B, B);
    Both (A, B);
    Views (A);
    Views (C);
    Copy (C, D);
    Put_Line (Integer'Image (C.X + C.Y + C.Z));
    Copy (C, C);
    Copy (A, C);
    Visit (Root'Class (D));
end ClasswideDispatch;
