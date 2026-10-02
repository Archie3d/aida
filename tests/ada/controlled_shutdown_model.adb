with Ada.Text_IO; use Ada.Text_IO;
package body Controlled_Shutdown_Model is
    procedure Initialize (Object : in out Guard) is
    begin
        Next_Id := Next_Id + 1;
        Object.Id := Next_Id;
        Put_Line ("initialize" & Integer'Image (Object.Id));
        if Object.Id = Fail_Initialize then
            raise Constraint_Error with "initialization failed";
        end if;
        Live := Live + 1;
    end Initialize;
    procedure Adjust (Object : in out Guard) is
    begin
        Live := Live + 1;
        Put_Line ("adjust" & Integer'Image (Object.Id));
    end Adjust;
    procedure Finalize (Object : in out Guard) is
    begin
        Put_Line ("finalize" & Integer'Image (Object.Id));
        Live := Live - 1;
        if Object.Id = Fail_Finalize then
            raise Constraint_Error with "finalization failed";
        end if;
    end Finalize;
    type Audit is new Ada.Finalization.Limited_Controlled with null record;
    overriding procedure Finalize (Object : in out Audit) is
    begin
        if Live /= 0 then
            Put_Line ("leaked objects" & Integer'Image (Live));
            raise Program_Error;
        end if;
        Put_Line ("shutdown clean");
    end Finalize;
    Last_To_Finalize : Audit;
end Controlled_Shutdown_Model;
