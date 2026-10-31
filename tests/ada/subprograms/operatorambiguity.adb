procedure OperatorAmbiguity is
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
    -- Both the predefined Number + Number and this user profile match.
    N := Pick + N;
    N := "+" (Pick, N);
end OperatorAmbiguity;
