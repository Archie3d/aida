with Ada.Finalization;
with System;
with Controlled_Result_Model;
package Controlled_Completion_Model is
    Live : Integer := 0;
    type Limited_Guard is new Ada.Finalization.Limited_Controlled with record
        Original : System.Address;
    end record;
    overriding procedure Initialize (Object : in out Limited_Guard);
    overriding procedure Finalize (Object : in out Limited_Guard);
    function Make return Limited_Guard;
    type Root is tagged null record;
    function Base_Item (Object : Root) return Controlled_Result_Model.Guard;
    function Select_Item (Object : Root) return Controlled_Result_Model.Guard;
    pragma Import (Ada, Select_Item, "controlled_completion_model__base_item");
    type Child is new Root with record
        Item : Controlled_Result_Model.Guard;
    end record;
    overriding function Select_Item (Object : Child) return Controlled_Result_Model.Guard;
    function Make_Array return Controlled_Result_Model.Guards;
    function Dynamic return Root'Class;
    type Audit is new Ada.Finalization.Limited_Controlled with null record;
    overriding procedure Finalize (Object : in out Audit);
end Controlled_Completion_Model;
