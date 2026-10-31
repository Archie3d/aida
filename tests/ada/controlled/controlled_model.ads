with Ada.Finalization;
package Controlled_Model is
    Live : Integer := 0;
    type Guard is new Ada.Finalization.Controlled with record
        Id : Integer := 0;
    end record;
    overriding procedure Initialize (Object : in out Guard);
    overriding procedure Finalize (Object : in out Guard);
end Controlled_Model;
