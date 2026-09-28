with Ada.Text_IO; use Ada.Text_IO;
procedure TaggedEquality is
    Calls : Integer := 0;
    package P is
        type Root is tagged record
            X : Integer;
        end record;
        function Make return Root;
        overriding function "=" (Left, Right : Root) return Boolean;
        type Child is new Root with record
            Y : Integer;
        end record;
        overriding function Make return Child;
        type Empty_Child is new Root with null record;
        type Leaf is new Child with null record;
    end P;
    package body P is
        function Make return Root is
        begin
            return (X => 2);
        end Make;
        function "=" (Left, Right : Root) return Boolean is
        begin
            Calls := Calls + 1;
            return Left.X mod 2 = Right.X mod 2;
        end "=";
        function Make return Child is
        begin
            return (Root'(X => 3) with Y => 4);
        end Make;
    end P;
    use P;
    A : Child := (Root'(X => 1) with Y => 5);
    B : Child := (Root'(X => 3) with Y => 5);
    C : Child := (Root'(X => 3) with Y => 7);
    D : Empty_Child := Make;
    E : Leaf := Make;
    type Container is record
        Value : Child;
    end record;
    type Items is array (1 .. 2) of Child;
    List : Items := (A, B);
begin
    Put_Line (Boolean'Image (A = B));
    Put_Line (Boolean'Image (A /= C));
    Put_Line (Integer'Image (Calls));
    Put_Line (Integer'Image (D.X));
    Put_Line (Integer'Image (E.X + E.Y));
    Put_Line (Integer'Image (List (2).Y));
end TaggedEquality;
