procedure DuplicateExceptionErrors is
    Failure : exception;
    fAiLuRe : exception;
    Pair, Pair : exception;
    Value : Integer;
    Value : exception;
    type Item is range 1 .. 10;
    Item : exception;
    procedure Action is null;
    Action : exception;
    Count : constant := 1;
    Count : exception;
    type Color is (Red, Blue);
    Red : exception;
    Alias : exception renames Failure;
    Alias : exception renames Constraint_Error;
    Count : exception renames Failure;
begin
    null;
end DuplicateExceptionErrors;
