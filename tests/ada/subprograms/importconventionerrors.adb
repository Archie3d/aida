pragma Import (Fortran, Context_Routine, "context_routine");
procedure ImportConventionErrors is
    procedure Foreign_Routine;
    pragma Import (Intrinsic, Foreign_Routine, "intrinsic_routine");
    pragma Import (Assembler, Foreign_Routine, "assembler_routine");
    pragma Import (COBOL, Foreign_Routine, "cobol_routine");
    pragma Import (CPP, Foreign_Routine, "cpp_routine");
    pragma Import (Stdcall, Foreign_Routine, "stdcall_routine");
    pragma Import (Java, Foreign_Routine, "java_routine");
    pragma Import (Unknown_Convention, Foreign_Routine, "unknown_routine");
    pragma Import (C, Foreign_Routine);
    pragma Import (aDa, Foreign_Routine);
    function Foreign_Function return Integer;
    pragma Import (c, Foreign_Function);
    function "**" (Left, Right : Integer) return Integer;
    pragma Import (Ada, "**");
    pragma Import (C, Foreign_Routine, "foreign_routine", "link_name");
    pragma Import (Ada, Foreign_Routine, "foreign_routine", "link_name");
    pragma Import (C, Foreign_Routine, "foreign_routine", Link_Name => "link_name");
    pragma Import (Convention => C, Entity => Foreign_Routine, External_Name => "foreign_routine");
    pragma Import (cOnVeNtIoN => Ada, Entity => Foreign_Routine, External_Name => "foreign_routine");
    pragma Import (C, Entity => Foreign_Routine, External_Name => "foreign_routine");
    pragma Import (C, Foreign_Routine, External_Name => "foreign_routine");
    pragma Import (Ada, Foreign_Routine, Link_Name => "link_name");
    pragma Import (Unknown_Argument => C, Foreign_Routine, "foreign_routine");
begin
    pragma Import (fOrTrAn, Foreign_Routine, "statement_routine");
    null;
end ImportConventionErrors;
