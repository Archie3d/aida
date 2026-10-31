with Ada.Finalization;
with Controlled_Result_Model; use Controlled_Result_Model;
package Controlled_Result_Globals is
    type Audit is new Ada.Finalization.Limited_Controlled with null record;
    overriding procedure Finalize (Object : in out Audit);
    Last_To_Finalize : Audit;
    Object : Guard := Make (42);
end Controlled_Result_Globals;
