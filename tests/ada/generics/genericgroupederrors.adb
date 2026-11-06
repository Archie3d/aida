procedure GenericGroupedErrors is
    generic
        A, B : Integer;
        X, Y : in out Integer;
    package Model is end Model;
    Value : Integer := 0;
    package Missing is new Model (A => 1, X => Value, Y => Value);
    package Duplicate is new Model (A => 1, B => 2, B => 3);
    package Wrong_Type is new Model (1, True, Value, Value);
    package Constant_Actual is new Model (1, 2, Value, 3);
begin
    null;
end GenericGroupedErrors;
