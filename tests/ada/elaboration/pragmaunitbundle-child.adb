pragma Assertion_Policy (Ignore);
separate (PragmaUnitBundle)
procedure Child is
begin
    pragma Assert (False);
end Child;
