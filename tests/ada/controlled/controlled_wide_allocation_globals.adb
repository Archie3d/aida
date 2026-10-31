with Ada.Text_IO; use Ada.Text_IO;
package body Controlled_Wide_Allocation_Globals is
    procedure Finalize (Object : in out Audit) is
    begin
        Check (Objects = 0 and Resources = 0);
        Put_Line ("global collection finalized");
    end Finalize;
    function Create return Guard_Link is
    begin
        return new Guard;
    end Create;
end Controlled_Wide_Allocation_Globals;
