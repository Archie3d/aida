procedure DiscriminantCategoryErrors is
    type Enum (D : Integer) is (One, Two);
    type Signed (D : Integer) is range 0 .. 10;
    type Unsigned (D : Integer) is mod 16;
    type Floating (D : Integer) is digits 6;
    type Fixed (D : Integer) is delta 0.125 range 0.0 .. 2.0;
    type Array_Type (D : Integer) is array (1 .. 2) of Integer;
    type Pointer (D : Integer) is access Integer;
    type Derived (D : Integer) is new Integer;
begin
    null;
end DiscriminantCategoryErrors;
