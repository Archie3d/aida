package Runtime_Layout_Collision is
    N : Integer := 3;
    type Spec_Record is record
        Data : String (1 .. N) := (others => 's');
    end record;
    Spec_Value : Spec_Record;
    procedure Check;
end Runtime_Layout_Collision;
