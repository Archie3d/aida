package body Runtime_Layout_Collision is
    -- Keep this constraint at the same line/column as the spec's constraint.
    type Body_Record is record
        Data : String (1 .. N) := (others => 'b');
    end record;
    Body_Value : Body_Record;
    procedure Check is
    begin
        if Spec_Value.Data /= "sss" or Body_Value.Data /= "bbb" then
            raise Program_Error;
        end if;
    end Check;
end Runtime_Layout_Collision;
