with Ada.Finalization;
with Controlled_Result_Model; use Controlled_Result_Model;
package Controlled_Allocation_Globals is
    type Audit is new Ada.Finalization.Limited_Controlled with null record;
    overriding procedure Finalize (Object : in out Audit);
    Last_To_Finalize : Audit;
    type Guard_Link is access Guard;
    Object : Guard_Link := new Guard'(Make (42));
    function Create return Guard_Link;
end Controlled_Allocation_Globals;
