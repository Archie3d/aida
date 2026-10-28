pragma Assertion_Policy (Ignore);
procedure PragmaIgnoredTypeErrors is
begin
    pragma Assert (Unknown_Name);
    pragma Assert (True, 123);
end PragmaIgnoredTypeErrors;
