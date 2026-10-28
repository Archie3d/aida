procedure GenericPackageBoxErrors is
    generic
        type T is private;
        N : Integer := 3;
    package Model is end Model;
    generic
        with package P is new Model (T => Integer, others => <>);
    package Partial_View is end Partial_View;
begin
    null;
end GenericPackageBoxErrors;
