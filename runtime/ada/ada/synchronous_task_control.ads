with Ada.Finalization;
with System;
package Ada.Synchronous_Task_Control is
    pragma Preelaborate (Synchronous_Task_Control);
    type Suspension_Object is limited private;
    pragma Preelaborable_Initialization (Suspension_Object);
    procedure Set_True (S : in out Suspension_Object);
    procedure Set_False (S : in out Suspension_Object);
    function Current_State (S : Suspension_Object) return Boolean;
    procedure Suspend_Until_True (S : in out Suspension_Object);
private
    type Suspension_Object is new Ada.Finalization.Limited_Controlled with record
        Handle : System.Address := null;
    end record;
    overriding procedure Finalize (S : in out Suspension_Object);
end Ada.Synchronous_Task_Control;
