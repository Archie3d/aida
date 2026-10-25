with Subunit_Dep; use Subunit_Dep;
separate (Subunits)
procedure Work (Value : in out Integer) is
    procedure Deep is separate;
begin
    Deep;
    Value := Nested.Add (Value) + Bonus;
end Work;
