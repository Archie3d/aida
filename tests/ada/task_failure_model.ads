with Ada.Finalization;
package Task_Failure_Model is
    type Guard is new Ada.Finalization.Limited_Controlled with null record;
    overriding procedure Finalize (object : in out Guard);
    task Worker;
    object : Guard;
end Task_Failure_Model;
