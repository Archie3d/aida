package RuntimeScalarLibraryErrors is
    N : Integer := 3;
    subtype Dynamic is Integer range 1 .. N;
    type Static_Only is range 1 .. Dynamic'Last;
end RuntimeScalarLibraryErrors;
