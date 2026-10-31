with Ada.Finalization;
package Controlled_Shutdown_Model is
    Live : Integer := 0;
    Next_Id : Integer := 0;
    Fail_Initialize : Integer := 0;
    Fail_Finalize : Integer := 0;
    type Guard is new Ada.Finalization.Controlled with record
        Id : Integer := 0;
    end record;
    overriding procedure Initialize (Object : in out Guard);
    overriding procedure Adjust (Object : in out Guard);
    overriding procedure Finalize (Object : in out Guard);
end Controlled_Shutdown_Model;
