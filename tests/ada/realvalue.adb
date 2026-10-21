with Ada.Text_IO;
with Real_Value_Checks;
procedure RealValue is
    procedure Check_Float is new Real_Value_Checks.Run (Float);
    procedure Check_Long is new Real_Value_Checks.Run (Long_Float);
    type Derived is new Long_Float;
    procedure Check_Derived is new Real_Value_Checks.Run (Derived);
    type Restricted is digits 6 range -1.0 .. 1.0;
    type Child is new Restricted;
    subtype Empty is Float range 2.0 .. 1.0;
    F : Float;
    L : Long_Float;
begin
    Check_Float;
    Check_Long;
    Check_Derived;
    if Restricted'Value ("2.0") /= 2.0
        or Child'Value ("2.0") /= 2.0
        or Empty'Value ("0.0") /= 0.0
    then
        raise Program_Error;
    end if;
    -- This lies just above a binary32 midpoint but rounds to that midpoint
    -- in binary64: converting first to double would round twice incorrectly.
    F := Float'Value ("1.0000000596046447753906250000000000000001");
    if F /= 1.0 + 2.0 ** (-23) then
        raise Program_Error;
    end if;
    L := Long_Float'Value ("1.00000000000000011102230246251565404236316680908203125");
    if L /= 1.0 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("real value");
end RealValue;
