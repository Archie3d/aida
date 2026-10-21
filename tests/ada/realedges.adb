with Ada.Text_IO;
with Real_Edge_Checks;
procedure RealEdges is
    procedure Check_Float is new Real_Edge_Checks.Run (Float);
    procedure Check_Long is new Real_Edge_Checks.Run (Long_Float);
    type Derived is new Long_Float;
    procedure Check_Derived is new Real_Edge_Checks.Run (Derived);
    Wide : Long_Float := Long_Float'Last;
    Narrow : Float;
begin
    Check_Float;
    Check_Long;
    Check_Derived;
    begin
        Narrow := Float (Wide);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    Ada.Text_IO.Put_Line ("real edge cases");
end RealEdges;
