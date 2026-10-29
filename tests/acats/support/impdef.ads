-- Local POSIX settings for CXAH001/CXAH002/CXAG001, not the full ACATS Impdef.
-- RunAcats supplies the readable value and removes the writable name.
package Impdef is
    Existing_Environment_Variable_To_Read : constant String := "AIDA_ACATS_READ";
    Unused_Environment_Variable_To_Modify : constant String := "AIDA_ACATS_WRITE";
    Directory_To_Create : constant String := "aida_acats_directory";
    function Equivalent_File_Names (Left, Right : String) return Boolean;
end Impdef;
