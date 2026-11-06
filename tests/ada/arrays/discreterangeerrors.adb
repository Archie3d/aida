procedure DiscreteRangeErrors is
    type Day is (Mon, Tue, Wed);
    type Other is (Red, Green, Blue);
    subtype Middle is Day range Tue .. Wed;
    type Vectors is array (Day range <>) of Integer;
    type Matrix is array (Integer range <>, Day range <>) of Integer;
    X : Vectors (Mon .. Wed) := (others => 0);
    G : Matrix (1 .. 2, Mon .. Wed) := (others => (others => 0));
    Dimension : Integer := 2;
    type Bad_Dimension is array (G'Range (Dimension)) of Integer;
    type Too_Many is array (G'Range (3)) of Integer;
    type Wrong_Index is array (Integer range X'Range) of Integer;
    subtype Wrong_Scalar is Integer range X'Range;
    Bad : Vectors (Other range Red .. Blue);
begin
    X := (Other range Red .. Blue => 0);
    X := (X'Range (2) => 0);
    X := (Float range 1 .. 2 => 0);
end DiscreteRangeErrors;
