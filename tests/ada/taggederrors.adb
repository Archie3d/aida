procedure TaggedErrors is
    type Plain is record
        X : Integer;
    end record;
    type Bad_Extension is new Plain with null record;
    type Root is tagged record
        X : Integer;
    end record;
    type Missing_With is new Root;
    type Child is new Root with record
        Y : Integer;
    end record;
    type Discriminated (N : Integer) is tagged null record;
    type Duplicate_Field is new Root with record
        X : Integer;
    end record;
    for Root'Size use 8;
    A : Root := (X => 1);
    B : Child := (Root with Y => 2);
    C : constant Child := (Root with Y => 3);
    Bad : Child := (X => 1, Y => 2);
    Bad_Ancestor : Child := (Plain with Y => 1);
    Repeated : Child := (A with X => 2, Y => 3);
    Too_Few : Child := (A with null record);
    Too_Many : Child := (A with Y => 1, Y => 2);
    Down : Child := Child (A);
    Wrong : Root := (A with null record);
    function Make return Child is
    begin
        return (Root with Y => 1);
    end Make;
    package Factory is
        type Base is tagged null record;
        function Make return Base;
        type Extended is new Base with record
            X : Integer;
        end record;
    end Factory;
begin
    Root (C).X := 1;
    Root (C) := A;
    Root (Make) := A;
end TaggedErrors;
