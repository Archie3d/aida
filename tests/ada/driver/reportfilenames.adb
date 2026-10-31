with Report;
with Ada.Text_IO;
procedure ReportFileNames is
    procedure Check (Value : Boolean) is
    begin
        if not Value then
            raise Program_Error;
        end if;
    end Check;
    Caught : Boolean := False;
begin
    Report.Test ("CXAG001", "File name adapter");
    Check (Report.Legal_File_Name = "XAG001");
    Check (Report.Legal_File_Name (2) = "YAG001");
    Check (Report.Legal_File_Name (3) = "ZAG001");
    Check (Report.Legal_File_Name (4) = "VAG001");
    Check (Report.Legal_File_Name (5) = "WAG001");
    Check (Report.Legal_File_Name (Nam => "C96006A") = "X6006A");
    begin
        Ada.Text_IO.Put_Line (Report.Legal_File_Name (0));
    exception
        when Constraint_Error => Caught := True;
    end;
    Check (Caught);
    Report.Test ("C96007A", "Updated test identity");
    Check (Report.Legal_File_Name = "X6007A");
    Report.Result;
end ReportFileNames;
