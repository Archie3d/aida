procedure Generic_Family.Reset (Value : in out Cell) is
begin
    Value.Value := Seed;
    Touches := Touches + 1;
end Generic_Family.Reset;
