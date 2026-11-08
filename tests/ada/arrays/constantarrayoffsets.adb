procedure ConstantArrayOffsets is
    type Values is array (Integer range <>) of Integer;
    type WideValues is array (Long_Integer range <>) of Integer;
    function Known return Integer is
        Items : Values (-3 .. 3) := (others => 17);
    begin
        return Items (-1);
    end Known;
    function Extreme return Integer is
        Items : WideValues (Long_Integer'First .. Long_Integer'First + 1) := (others => 19);
    begin
        return Items (Long_Integer'First + 1);
    end Extreme;
    function Dynamic (Index : Integer) return Integer is
        Items : Values (1 .. 4) := (others => 23);
    begin
        return Items (Index);
    end Dynamic;
    function NullRange return Integer is
        Items : Values (4 .. 3) := (others => 0);
    begin
        return Items (4);
    end NullRange;
    type Flags is array (1 .. 8) of Boolean;
    pragma Pack (Flags);
    function Packed return Boolean is
        Items : Flags := (others => True);
    begin
        return Items (2);
    end Packed;
begin
    if Known /= 17 or Extreme /= 19 or Dynamic (2) /= 23 or not Packed then
        raise Program_Error;
    end if;
    begin
        if Dynamic (5) = 23 then null; end if;
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    begin
        if NullRange = 0 then null; end if;
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
end ConstantArrayOffsets;
