with Ada.Task_Identification;
with Ada.Exceptions;
package Ada.Task_Termination is
    pragma Preelaborate (Task_Termination);
    type Cause_Of_Termination is (Normal, Abnormal, Unhandled_Exception);
    type Termination_Handler is access protected procedure
        (Cause : in Cause_Of_Termination;
         T : in Ada.Task_Identification.Task_Id;
         X : in Ada.Exceptions.Exception_Occurrence);
    procedure Set_Dependents_Fallback_Handler (Handler : in Termination_Handler);
    pragma Import (C, Set_Dependents_Fallback_Handler, "__ada_set_dependents_handler");
    function Current_Task_Fallback_Handler return Termination_Handler;
    pragma Import (C, Current_Task_Fallback_Handler, "__ada_current_fallback_handler");
    procedure Set_Specific_Handler
        (T : in Ada.Task_Identification.Task_Id; Handler : in Termination_Handler);
    pragma Import (C, Set_Specific_Handler, "__ada_set_specific_handler");
    function Specific_Handler (T : Ada.Task_Identification.Task_Id) return Termination_Handler;
    pragma Import (C, Specific_Handler, "__ada_specific_handler");
end Ada.Task_Termination;
