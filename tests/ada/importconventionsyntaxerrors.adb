procedure ImportConventionSyntaxErrors is
    procedure Foreign_Routine;
    pragma Import;
    pragma Import (, Foreign_Routine, "foreign_routine");
    pragma Import ("C", Foreign_Routine, "foreign_routine");
    pragma Import (C Foreign_Routine, "foreign_routine");
begin
    null;
end ImportConventionSyntaxErrors;
