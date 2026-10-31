procedure ImportConventionSyntaxErrors is
    procedure Foreign_Routine;
    pragma Import;
    pragma Import (, Foreign_Routine, "foreign_routine");
    pragma Import ("C", Foreign_Routine, "foreign_routine");
    pragma Import (C Foreign_Routine, "foreign_routine");
    pragma Import (C, Foreign_Routine, "");
    pragma Import (Ada, Foreign_Routine, "");
    pragma Import (C, Foreign_Routine, );
    pragma Import (Ada, Foreign_Routine, 123);
    pragma Import (C, , "foreign_routine");
    pragma Import (C, Foreign_Routine, "foreign_routine";
    pragma Import (C, Foreign_Routine, "foreign_routine"));
    pragma Import (C, Foreign_Routine, "foreign" & "_routine");
    pragma Import (C, Foreign_Routine, "foreign_routine") Extra;
    pragma Import (C, Foreign_Routine, "foreign_routine", );
begin
    null;
end ImportConventionSyntaxErrors;
