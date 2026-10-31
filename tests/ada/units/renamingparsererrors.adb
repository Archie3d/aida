procedure RenamingParserErrors is
    package Bad renames;
    generic procedure Bad_Generic renames;
    generic function With_Profile (X : Integer) return Integer renames Missing;
    generic
        type Item is private;
    package With_Formals renames Missing;
    package Call_Target renames Missing (1);
begin
    null;
end RenamingParserErrors;
