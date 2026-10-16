procedure AggregateAuditErrors is
    type Pair is record
        Left, Right : Integer := 0;
    end record;
    type Mixed is record
        Number : Integer;
        Real_Number : Float;
    end record;
    type Other_Integer is new Integer;
    type Distinct is record
        Left : Integer;
        Right : Other_Integer;
    end record;
    type Vector is array (1 .. 3) of Integer;
    Missing : Pair := (Left => 1);
    Duplicate : Pair := (Left => 1, Left => 2, Right => 3);
    Order : Pair := (Left => 1, 2);
    Early_Others : Pair := (others => 1, Right => 2);
    Repeated_Others : Pair := (others => 1, others => 2);
    Empty_Others : Pair := (Left => 1, Right => 2, others => 3);
    Numeric_Choice : Pair := (1 => 2, Right => 3);
    Range_Choice : Pair := (Left .. Right => 2);
    Heterogeneous : Mixed := (others => 1);
    Derived_Components : Distinct := (Left | Right => 1);
    Overlap : Vector := (1 .. 2 => 1, 2 .. 3 => 2);
    Hole : Vector := (1 => 1, 3 => 3);
    Array_Order : Vector := (others => 1, 1 => 2);
begin
    null;
end AggregateAuditErrors;
