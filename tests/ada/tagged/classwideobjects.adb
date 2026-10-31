with Ada.Text_IO; use Ada.Text_IO;
with Ada.Tags; use Ada.Tags;
with Ada.Unchecked_Deallocation;
procedure ClasswideObjects is
    package P is
        type Root is tagged record
            X : Integer := 2;
        end record;
        function New_Value return Root;
        function With_Value (N : Integer) return Root;
        function Clone (Item : Root) return Root;
        function "+" (Left, Right : Root) return Root;
        type Child is new Root with record
            Y : Integer := 40;
        end record;
        overriding function New_Value return Child;
        overriding function With_Value (N : Integer) return Child;
        overriding function Clone (Item : Child) return Child;
        overriding function "+" (Left, Right : Child) return Child;
    end P;
    package body P is
        function New_Value return Root is
        begin
            return (X => 10);
        end New_Value;
        function With_Value (N : Integer) return Root is
        begin
            return (X => N);
        end With_Value;
        function New_Value return Child is
        begin
            return (Root'(X => 11) with Y => 22);
        end New_Value;
        function With_Value (N : Integer) return Child is
        begin
            return (Root'(X => N) with Y => N + 1);
        end With_Value;
        function Clone (Item : Root) return Root is
        begin
            return Item;
        end Clone;
        function "+" (Left, Right : Root) return Root is
        begin
            return (X => Left.X + Right.X);
        end "+";
        function Clone (Item : Child) return Child is
        begin
            return Item;
        end Clone;
        function "+" (Left, Right : Child) return Child is
        begin
            return (Root'(X => Left.X + Right.X) with Y => Left.Y + Right.Y);
        end "+";
    end P;
    use P;
    function Make return Root'Class is
        Local : Child := (Root'(X => 3) with Y => 50);
    begin
        return Local;
    end Make;
    A : Root'Class := Make;
    B : Root'Class := Clone (A);
    C : Root'Class := A + B;
    R : Root'Class := Root'(X => 3);
    Initial : Root'Class := Root'(P.New_Value);
    type Link is access Root'Class;
    procedure Free is new Ada.Unchecked_Deallocation (Root'Class, Link);
    Pointer : Link := new Root'Class'(Make);
    type Holder is record
        Item : Link;
    end record;
    H : Holder := (Item => new Child'(Root'(X => 7) with Y => 8));
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
begin
    Check (Initial'Tag = Root'Tag and Initial.X = 10);
    Check (A = B);
    Child (B).Y := 99;
    Check (A /= B);
    Check (A /= R);
    Check (Child (A).Y = 50);
    Check (Child (C).Y = 100 and C.X = 6);
    A := A;
    A := Root'(P.New_Value);
    Check (Child (A).Y = 22 and A.X = 11);
    A := Root'(With_Value (30));
    Check (Child (A).Y = 31 and A.X = 30);
    A := A + P.New_Value;
    Check (Child (A).Y = 53 and A.X = 41);
    A := Root'(Clone (P.New_Value));
    Check (Child (A).Y = 22);
    Check (A'Tag = Child'Tag);
    Check (Parent_Tag (A'Tag) = Root'Tag);
    Check (Parent_Tag (Root'Tag) = No_Tag);
    Check (Internal_Tag (External_Tag (A'Tag)) = A'Tag);
    Check (Descendant_Tag (External_Tag (A'Tag), Root'Tag) = A'Tag);
    Check (Is_Descendant_At_Same_Level (A'Tag, Root'Tag));
    Check (not Is_Abstract (A'Tag));
    Check (Expanded_Name (A'Tag) = "CLASSWIDEOBJECTS.P.CHILD");
    Check (Child'External_Tag = External_Tag (A'Tag));
    Check (Interface_Ancestor_Tags (A'Tag)'Length = 0);
    Check (Child (Pointer.all).Y = 50);
    Check (Child (H.Item.all).Y = 8);
    Free (Pointer);
    Free (H.Item);
    Check (Pointer = null and H.Item = null);
    declare
        N : Tag := No_Tag;
    begin
        N := Parent_Tag (No_Tag);
        raise Program_Error;
    exception
        when Tag_Error => Put_Line ("tag error");
    end;
    Put_Line ("class-wide ownership ok");
end ClasswideObjects;
