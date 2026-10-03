with Controlled_Wide_Allocation_Globals; use Controlled_Wide_Allocation_Globals;
with Controlled_Result_Model; use Controlled_Result_Model;
with Ada.Unchecked_Deallocation;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledWideAllocationGlobals is
    procedure Free is new Ada.Unchecked_Deallocation (Guard'Class, Guard_Link);
    Local : Guard_Link := Create;
begin
    Check (Objects = 2 and Resources = 2 and Object.Data.Value = 42);
    Free (Object);
    Check (Object = null and Objects = 1 and Resources = 1);
    Put_Line ("global collection alive");
end ControlledWideAllocationGlobals;
