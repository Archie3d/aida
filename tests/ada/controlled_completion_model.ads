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
    type Child is new Root with record
        Item : Controlled_Result_Model.Guard;
    end record;
    function Dynamic return Root'Class;
    type Audit is new Ada.Finalization.Limited_Controlled with null record;
    overriding procedure Finalize (Object : in out Audit);
    Last : Audit;
    Limited_Object : Limited_Guard := Make;
    Dynamic_Object : Root'Class := Dynamic;
    type Link is access Root'Class;
    Heap : Link := new Root'Class'(Dynamic);
end Controlled_Completion_Model;
