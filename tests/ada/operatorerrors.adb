procedure OperatorErrors is
    function "+" return Integer;
    function "-" (A, B, C : Integer) return Integer;
    function "abs" (A, B : Integer) return Integer;
    function "not" (A : out Integer) return Integer;
    function "*" (A : Integer; B : Integer := 1) return Integer;
    function "=" (A, B : Integer) return Boolean;
    function "/=" (A, B : Integer) return Boolean;
    type Box is record
        Value : Integer;
    end record;
    function "+" (A, B : Box) return Box is
    begin
        return A;
    end "+";
    function "+" (A, B : Box) return Integer is
    begin
        return A.Value;
    end "+";
    procedure Take (Value : Box) is
    begin
        null;
    end Take;
    procedure Take (Value : Integer) is
    begin
        null;
    end Take;
    Left, Right : Box := (Value => 1);
    Value : Integer;
begin
    Take (Left + Right);
    Value := Left * Right;
end OperatorErrors;
