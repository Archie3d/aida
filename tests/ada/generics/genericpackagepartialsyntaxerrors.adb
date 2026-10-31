procedure GenericPackagePartialSyntaxErrors is
    generic
        type T is private;
        N : Integer := 3;
    package Model is end Model;
    generic
        with package P is new Model (<> , N => 3);
    package Positional_Box is end Positional_Box;
    generic
        with package P is new Model (others => <>, N => 3);
    package Others_Not_Last is end Others_Not_Last;
    generic
        with package P is new Model (T => Integer, <>);
    package Unnamed_Box is end Unnamed_Box;
    generic
        with package P is new Model (others => 3);
    package Others_Value is end Others_Value;
begin
    null;
end GenericPackagePartialSyntaxErrors;
