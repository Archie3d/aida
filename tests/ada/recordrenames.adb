with Ada.Text_IO; use Ada.Text_IO;
with Renamed_Fields;
procedure RecordRenames is
    type Pair is record
        X : Integer;
        Y : Integer;
    end record;
    type Box is record
        Item : Pair;
    end record;
    R : Box := (Item => (X => 1, Y => 2));
    X : Integer renames R.Item.X;
    Item : Pair renames R.Item;
    Again : Integer renames X;
    Fixed : constant Pair := (X => 7, Y => 8);
    Read_Only : Integer renames Fixed.X;
    type Pairs is array (1 .. 2) of Pair;
    Values : Pairs := (1 => (X => 10, Y => 11), 2 => (X => 20, Y => 21));
    Calls : Integer := 0;
    function Index return Integer is
    begin
        Calls := Calls + 1;
        return Calls;
    end Index;
    Selected : Integer renames Values(Index).X;
    type Pair_Access is access Pair;
    P : Pair_Access := new Pair'(X => 40, Y => 41);
    Original : Pair_Access := P;
    Through_Pointer : Integer renames P.X;
    procedure Bump(V : in out Integer) is
    begin
        V := V + 1;
    end Bump;
    procedure Nested is
    begin
        Again := Again + 3;
    end Nested;
begin
    Renamed_Fields.X := 9;
    Renamed_Fields.Text(2) := 'Z';
    Put_Line(Integer'Image(Renamed_Fields.R.X));
    Put_Line(Renamed_Fields.R.Text);
    P := new Pair'(X => 50, Y => 51);
    Through_Pointer := 42;
    Put_Line(Integer'Image(Original.X));
    Put_Line(Integer'Image(P.X));
    X := 4;
    Nested;
    Bump(X);
    Put_Line(Integer'Image(R.Item.X));
    Item := (X => 12, Y => 13);
    Put_Line(Integer'Image(X));
    R.Item.X := 15;
    Put_Line(Integer'Image(Again));
    Put_Line(Integer'Image(Read_Only));
    Selected := 30;
    Selected := Selected + 1;
    Put_Line(Integer'Image(Values(1).X));
    Put_Line(Integer'Image(Values(2).X));
    Put_Line(Integer'Image(Calls));
end RecordRenames;
