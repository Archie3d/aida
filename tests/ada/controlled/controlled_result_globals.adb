with Ada.Text_IO; use Ada.Text_IO;
package body Controlled_Result_Globals is
    procedure Finalize (Object : in out Audit) is
    begin
        Check (Objects = 0 and Resources = 0);
        Put_Line ("global result finalized");
    end Finalize;
end Controlled_Result_Globals;
