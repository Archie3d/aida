with Ada.Unchecked_Deallocation;
package body Controlled_Result_Model is
    procedure Free is new Ada.Unchecked_Deallocation (Resource, Link);
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error with "resource accounting failed";
        end if;
    end Check;
    procedure Initialize (Object : in out Guard) is
    begin
        Object.Data := new Resource'(1, 0);
        Resources := Resources + 1;
        Objects := Objects + 1;
    end Initialize;
    procedure Adjust (Object : in out Guard) is
    begin
        if Fail_Adjust > 0 then
            Fail_Adjust := Fail_Adjust - 1;
            if Fail_Adjust = 0 then
                raise Constraint_Error;
            end if;
        end if;
        Check (Object.Data /= null and then Object.Data.References > 0);
        Object.Data.References := Object.Data.References + 1;
        Objects := Objects + 1;
    end Adjust;
    procedure Finalize (Object : in out Guard) is
    begin
        Check (Object.Data /= null and then Object.Data.References > 0);
        Object.Data.References := Object.Data.References - 1;
        if Object.Data.References = 0 then
            Free (Object.Data);
            Resources := Resources - 1;
        end if;
        Object.Data := null;
        Objects := Objects - 1;
        if Fail_Finalize then
            Fail_Finalize := False;
            raise Constraint_Error;
        end if;
    end Finalize;
    function Make (Value : Integer) return Guard is
        Local : Guard;
    begin
        Local.Data.Value := Value;
        return Local;
    end Make;
end Controlled_Result_Model;
