procedure VisibleNumericAmbiguity is
    type Span is range 0 .. 100;
    function "*" (Left : Integer; Right : Span) return Span is
    begin
        return Right;
    end "*";
    function "/" (Left, Right : Span) return Integer is
    begin
        return 1;
    end "/";
    Rows : Natural := 24;
    Width : Natural := Rows * 3 / 4;
begin
    null;
end VisibleNumericAmbiguity;
