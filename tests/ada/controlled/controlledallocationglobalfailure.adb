with Controlled_Allocation_Globals; use Controlled_Allocation_Globals;
with Controlled_Result_Model; use Controlled_Result_Model;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledAllocationGlobalFailure is
    Local : Guard_Link := Create;
begin
    Check (Objects = 2 and Resources = 2);
    Fail_Finalize := True;
    Put_Line ("global collection ready");
end ControlledAllocationGlobalFailure;
