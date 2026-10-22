procedure NamedOperatorErrors is
    A, B : Integer := 1;
    Value : Integer;
    Flag : Boolean;
    type Number is range 0 .. 100;
    type Other is range 0 .. 100;
    function Pick return Number is
    begin
        return 1;
    end Pick;
    function Pick return Other is
    begin
        return 2;
    end Pick;
    function "+" (Left : Other; Right : Number) return Number is
    begin
        return Right;
    end "+";
    N : Number := 1;
begin
    N := "+" (Right => N, Left => Pick);
    Value := "rem" (Left => A, Left => B);
    Value := "mod" (Left => A, B);
    Value := "rem" (Dividend => A, Divisor => B);
    Value := "mod" (Left => A);
    Value := "-" (Left => A);
    Value := "abs" (Left => A, Right => B);
    Value := "rem" (Left => True, Right => B);
    Flag := "not" (Left => True);
    Value := "rem" (Left => A, Right => B, Extra => B);
    declare
        function "rem" (Dividend, Divisor : Integer) return Integer is
        begin
            return 42;
        end "rem";
    begin
        -- The explicit homograph hides the predefined Left/Right profile.
        Value := "rem" (Left => A, Right => B);
    end;
end NamedOperatorErrors;
