procedure OperatorVisibilityErrors is
    package P is
        type Number is new Integer;
        type Token is private;
        function Make return Token;
    private
        type Token is new Integer;
        function "and" (Left, Right : Number) return Number;
    end P;
    package body P is
        function Make return Token is
        begin
            return 1;
        end Make;
        function "and" (Left, Right : Number) return Number is
        begin
            return Left;
        end "and";
    end P;
    package Alias_View is
        subtype Number is P.Number;
    end Alias_View;
    package Other is
        function "+" (Left, Right : P.Number) return P.Number;
    end Other;
    package body Other is
        function "+" (Left, Right : P.Number) return P.Number is
        begin
            return Left;
        end "+";
    end Other;
    X : P.Number := 1;
begin
    X := X + X;
    X := Alias_View."+" (X, X);
    X := P."+" (First => X, Second => X);
    X := P."and" (X, X);
    declare
        use P;
        use Other;
    begin
        X := X + X;
    end;
    declare
        use type P.Number;
        function "+" (A, B : P.Number) return P.Number is
        begin
            return A;
        end "+";
    begin
        X := "+" (Left => X, Right => X);
    end;
    if P."+" (P.Make, P.Make) = P.Make then
        null;
    end if;
end OperatorVisibilityErrors;
