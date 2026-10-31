procedure AggregateBoxErrors is
    type Item is record
        Value : Integer := 1;
    end record;
    type Required (D : Integer) is record
        Value : Integer := 1;
    end record;
    type Variant (D : Boolean) is record
        case D is
            when True => Value : Integer := 1;
            when False => Other : Boolean := False;
        end case;
    end record;
    subtype Fixed is Variant (True);
    type Matrix is array (1 .. 2, 1 .. 2) of Integer;
    type Empty is null record;
    G : Empty := (others => <>);
    A : Item := (Value => <>, Value => <>);
    B : Required := (others => <>);
    C : Fixed := (others => <>);
    D : Matrix := (others => <>);
    E : Item := (Unknown => <>);
    F : Item := (others => <>, Value => 1);
begin
    null;
end AggregateBoxErrors;
