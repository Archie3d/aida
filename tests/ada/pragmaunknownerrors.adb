procedure pragmaunknownerrors is
    pragma Typo_Pragma;
    pragma Inlien (Run);
begin
    pragma Unknown_Statement (Nested (1, 2));
    null;
end pragmaunknownerrors;
