with Ada.Text_IO; use Ada.Text_IO;
procedure Filesnapshots is
    First, Second, Saved : File_Type;
    procedure Check_Names (A, B : String) is
    begin
        if A /= "snapshot-first.tmp" or B /= "snapshot-second.tmp" then
            raise Program_Error with "file names not independent";
        end if;
    end Check_Names;
begin
    Create (First, Out_File, "snapshot-first.tmp");
    Create (Second, Out_File, "snapshot-second.tmp");
    Set_Output (First);
    Saved := Current_Output;
    Set_Output (Second);
    Put_Line (Saved, "saved selection");
    Check_Names (Name (First), Name (Second));
    declare
        Old_Name : String := Name (First);
    begin
        Delete (First);
        Create (First, Out_File, "reused-slot.tmp");
        if Old_Name /= "snapshot-first.tmp" then
            raise Program_Error;
        end if;
    end;
    Delete (First);
    Delete (Second);
    Put_Line ("file snapshots ok");
end Filesnapshots;
