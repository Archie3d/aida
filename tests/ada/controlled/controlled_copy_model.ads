with Ada.Finalization;
package Controlled_Copy_Model is
    Live : Integer := 0;
    type Guard is new Ada.Finalization.Controlled with record
        Value : Integer := 42;
    end record;
    overriding procedure Initialize (Object : in out Guard);
    overriding procedure Adjust (Object : in out Guard);
    overriding procedure Finalize (Object : in out Guard);
    procedure Assign (Target : in out Guard; Source : Guard);
end Controlled_Copy_Model;
