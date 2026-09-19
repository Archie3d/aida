procedure Handlerchoiceerrors is
    E : exception;
    Alias_E : exception renames E;
begin
    begin
        null;
    exception
        when E => null;
        when Alias_E => null;
    end;
    begin
        null;
    exception
        when others => null;
        when E => null;
    end;
    begin
        null;
    exception
        when E | others => null;
    end;
    begin
        null;
    exception
        when others | E => null;
    end;
    begin
        null;
    exception
        when Constraint_Error => null;
        when Numeric_Error => null;
    end;
end Handlerchoiceerrors;
